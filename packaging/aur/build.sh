#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"

echo "=== Building AUR package for mx3 $VERSION ==="

mkdir -p "$SCRIPT_DIR/pkg"

podman build -t mx3-aur "$SCRIPT_DIR"

podman run --rm --entrypoint "" \
    -v "$PROJECT_DIR:/src:Z" \
    -v "$SCRIPT_DIR/pkg:/pkg:Z" \
    mx3-aur \
    /bin/sh -c "
        set -euo pipefail
        cp /src/PKGBUILD /src/.SRCINFO /home/builder/
        chown builder:builder /home/builder/PKGBUILD /home/builder/.SRCINFO
        cd /home/builder
        sudo -u builder makepkg -sf --noconfirm
    "

echo ""
echo "=== AUR package built ==="