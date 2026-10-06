#!/bin/bash
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$ROOT_DIR/:$PATH"
source "$ROOT_DIR/poky/oe-init-build-env" "$BUILDDIR"

cp ../meta-zero/templates/bblayers.conf conf/bblayers.conf
cp ../meta-zero/templates/local.conf conf/local.conf