#!/bin/sh
# Resize root partition + ext4 filesystem to fill the SD card on first boot (BusyBox/SysV)
set -eu

FLAG=/var/lib/resize-rootfs.done
LOG=/var/log/resize-rootfs.log

mkdir -p /var/lib /var/log

# Start logging as early as possible
exec >>"$LOG" 2>&1
set -x

echo "=== resize-rootfs: start ==="
date || true

# If already done, exit
if [ -e "$FLAG" ]; then
  echo "Flag exists ($FLAG). Nothing to do."
  exit 0
fi

# Ensure root is writable (common early-boot issue)
# Try to remount / as rw; ignore failure (some setups handle later)
mountpoint -q / 2>/dev/null || true
mount -o remount,rw / 2>/dev/null || true

# Detect root block device (prefer findmnt, fallback to /proc/mounts)
ROOTSRC=""
if command -v findmnt >/dev/null 2>&1; then
  ROOTSRC="$(findmnt -n -o SOURCE / 2>/dev/null || true)"
fi
if [ -z "$ROOTSRC" ] && [ -r /proc/mounts ]; then
  ROOTSRC="$(awk '$2=="/"{print $1; exit}' /proc/mounts 2>/dev/null || true)"
fi

echo "ROOTSRC=$ROOTSRC"

# Resolve common indirect root sources like /dev/root and PARTUUID=...
if [ "$ROOTSRC" = "/dev/root" ] || [ "$ROOTSRC" = "rootfs" ]; then
  RESOLVED="$(readlink -f "$ROOTSRC" 2>/dev/null || true)"
  [ -n "$RESOLVED" ] && ROOTSRC="$RESOLVED"
fi

if echo "$ROOTSRC" | grep -q '^PARTUUID='; then
  ROOT_PARTUUID="${ROOTSRC#PARTUUID=}"
  if [ -e "/dev/disk/by-partuuid/$ROOT_PARTUUID" ]; then
    ROOTSRC="$(readlink -f "/dev/disk/by-partuuid/$ROOT_PARTUUID" 2>/dev/null || true)"
  fi
fi

if [ -n "$ROOTSRC" ] && [ ! -b "$ROOTSRC" ] && command -v findmnt >/dev/null 2>&1; then
  ROOTSRC="$(findmnt -n -o SOURCE -T / 2>/dev/null || true)"
fi

if [ "$ROOTSRC" = "/dev/root" ] || [ "$ROOTSRC" = "rootfs" ]; then
  RESOLVED="$(readlink -f "$ROOTSRC" 2>/dev/null || true)"
  [ -n "$RESOLVED" ] && ROOTSRC="$RESOLVED"
fi

echo "ROOTSRC_RESOLVED=$ROOTSRC"

case "$ROOTSRC" in
  /dev/*) ;;
  *)
    echo "Root source is not a block device (ROOTSRC=$ROOTSRC). Exiting."
    exit 0
    ;;
esac

# Only handle ext4 safely (resize2fs). If not ext4, exit.
FSTYPE=""
if command -v findmnt >/dev/null 2>&1; then
  FSTYPE="$(findmnt -n -o FSTYPE / 2>/dev/null || true)"
fi
if [ -z "$FSTYPE" ] && [ -r /proc/mounts ]; then
  FSTYPE="$(awk '$2=="/"{print $3; exit}' /proc/mounts 2>/dev/null || true)"
fi
echo "FSTYPE=$FSTYPE"

if [ "$FSTYPE" != "ext4" ] && [ -n "$FSTYPE" ]; then
  echo "Unsupported filesystem type for auto-resize: $FSTYPE (expected ext4). Exiting."
  exit 0
fi

# Figure out disk + partition number: mmcblk0p2 vs sda2, nvme0n1p2, etc.
DISK=""
PARTNUM=""

case "$ROOTSRC" in
  /dev/mmcblk*p[0-9]*|/dev/nvme*n*p[0-9]*)
    DISK="$(echo "$ROOTSRC" | sed -E 's/p[0-9]+$//')"
    PARTNUM="$(echo "$ROOTSRC" | sed -E 's/.*p([0-9]+)$/\1/')"
    ;;
  /dev/disk/by-partuuid/*)
    ROOTSRC="$(readlink -f "$ROOTSRC" 2>/dev/null || true)"
    ;;
  /dev/*[0-9])
    DISK="$(echo "$ROOTSRC" | sed -E 's/[0-9]+$//')"
    PARTNUM="$(echo "$ROOTSRC" | sed -E 's/.*([0-9]+)$/\1/')"
    ;;
  *)
    echo "Could not parse disk/partition from ROOTSRC=$ROOTSRC"
    exit 1
    ;;
esac

if [ -z "$DISK" ] || [ -z "$PARTNUM" ]; then
  case "$ROOTSRC" in
    /dev/mmcblk*p[0-9]*|/dev/nvme*n*p[0-9]*)
      DISK="$(echo "$ROOTSRC" | sed -E 's/p[0-9]+$//')"
      PARTNUM="$(echo "$ROOTSRC" | sed -E 's/.*p([0-9]+)$/\1/')"
      ;;
    /dev/*[0-9])
      DISK="$(echo "$ROOTSRC" | sed -E 's/[0-9]+$//')"
      PARTNUM="$(echo "$ROOTSRC" | sed -E 's/.*([0-9]+)$/\1/')"
      ;;
  esac
fi

echo "DISK=$DISK PARTNUM=$PARTNUM"

# Basic sanity checks
if [ ! -b "$DISK" ] || [ ! -b "$ROOTSRC" ]; then
  echo "Block device check failed. DISK=$DISK ROOTSRC=$ROOTSRC"
  exit 1
fi

# Show current partition table (best effort)
if command -v parted >/dev/null 2>&1; then
  parted -s "$DISK" unit s print || true
fi

# Expand partition to 100%
# Do NOT swallow errors here; if it fails, we want to know and not set the flag.
if command -v growpart >/dev/null 2>&1; then
  echo "Using growpart to expand partition..."
  growpart "$DISK" "$PARTNUM"
else
  echo "Using parted to expand partition..."
  if ! command -v parted >/dev/null 2>&1; then
    echo "parted not found and growpart not found. Cannot resize partition."
    exit 1
  fi
  parted -s "$DISK" "resizepart $PARTNUM 100%"
fi

# Re-read partition table (best effort)
if command -v partprobe >/dev/null 2>&1; then
  partprobe "$DISK" || true
fi
sleep 2

# Expand filesystem (ext4 online)
if ! command -v resize2fs >/dev/null 2>&1; then
  echo "resize2fs not found. Install e2fsprogs-resize2fs."
  exit 1
fi

echo "Running resize2fs on $ROOTSRC..."
resize2fs "$ROOTSRC"

# Create flag only after success
touch "$FLAG"
sync || true

echo "=== resize-rootfs: done ==="
date || true
exit 0
