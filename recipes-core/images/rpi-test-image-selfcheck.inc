# Boot selfcheck diagnostics
ROOTFS_POSTPROCESS_COMMAND:append = " setup_boot_selfcheck_service; "

setup_boot_selfcheck_service() {
    install -d ${IMAGE_ROOTFS}/usr/bin
    cat > ${IMAGE_ROOTFS}/usr/bin/boot-selfcheck.sh << 'SCRIPT'
#!/bin/sh
LOG=/var/log/boot-selfcheck.log

{
  echo "=== Boot selfcheck $(date '+%Y-%m-%d %H:%M:%S') ==="
  echo "PID1=$(cat /proc/1/comm 2>/dev/null || true)"
  echo "ROOT=$(findmnt -n -o SOURCE / 2>/dev/null || true)"
  echo "ROOT_FS=$(findmnt -n -o FSTYPE / 2>/dev/null || true)"
  echo "DF_ROOT=$(df -h / 2>/dev/null | tail -n 1 || true)"
  echo "GROUPS=$(id zero 2>/dev/null || true)"
  echo "DOCKER_SOCK=$(ls -l /var/run/docker.sock 2>/dev/null || true)"
  echo "DOCKER_SOCK_NUM=$(stat -c '%u:%g %a' /var/run/docker.sock 2>/dev/null || true)"
  echo "docker_active=$(systemctl is-active docker 2>/dev/null || true)"
  echo "resize_status=$(systemctl is-active resize-rootfs 2>/dev/null || true)"
  echo "networkmanager_active=$(systemctl is-active NetworkManager 2>/dev/null || true)"
  echo "resolv_conf=$(grep nameserver /etc/resolv.conf 2>/dev/null | tr '\n' ' ')"
  echo "docker_ps_root=$(docker ps >/dev/null 2>&1; echo $?)"
  echo "docker_ps=$(su -s /bin/sh zero -c 'docker ps >/dev/null 2>&1'; echo $?)"
} >> "$LOG" 2>&1
SCRIPT
    chmod 0755 ${IMAGE_ROOTFS}/usr/bin/boot-selfcheck.sh

    install -d ${IMAGE_ROOTFS}${systemd_unitdir}/system
    cat > ${IMAGE_ROOTFS}${systemd_unitdir}/system/boot-selfcheck.service << 'EOF'
[Unit]
Description=Boot Selfcheck
After=network-online.target docker.service
Wants=network-online.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/bin/boot-selfcheck.sh

[Install]
WantedBy=multi-user.target
EOF

    install -d ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/multi-user.target.wants
    ln -sf ${systemd_unitdir}/system/boot-selfcheck.service \
        ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/multi-user.target.wants/boot-selfcheck.service
}
