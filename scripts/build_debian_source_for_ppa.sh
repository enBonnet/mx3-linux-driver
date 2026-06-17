#!/bin/bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PARENT_DIR="$(dirname "$PROJECT_DIR")"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"

echo "Pulling Debian container..."
podman pull debian:trixie-slim

echo "Building Debian source package inside container..."
podman run --rm \
    -v "$PARENT_DIR:/workspace:Z" \
    debian:trixie-slim \
    bash -c '
        set -euo pipefail
        apt-get update -qq
        apt-get install -y -qq build-essential devscripts debhelper-compat

        SRC="/workspace/mx3-linux-driver"
        TMP="/tmp"
        PKG="mx3-'"$VERSION"'"
        ORIG="$TMP/mx3_'"$VERSION"'.orig.tar.gz"

        # --- 1. Clean slate ---
        rm -rf "$TMP/$PKG" "$TMP/${PKG}-build" "$ORIG"

        # --- 2. Upstream source (no debian/, no dist/, no artifacts) ---
        cp -a "$SRC" "$TMP/$PKG"
        cd "$TMP/$PKG"
        rm -rf .git debian dist mx3 src/*.o tests/build scripts/build_debian_source_for_ppa.sh
        cd "$TMP"
        tar czf "$ORIG" "$PKG"

        # --- 3. Debian build tree (debian/ included, still no dist/) ---
        cp -a "$SRC" "$TMP/${PKG}-build"
        cd "$TMP/${PKG}-build"
        rm -rf .git dist mx3 src/*.o tests/build scripts/build_debian_source_for_ppa.sh

        # --- 4. Build source package ---
        # orig tarball is already in /tmp (parent of build tree), where dpkg-buildpackage looks
        cd "$TMP/${PKG}-build"
        dpkg-buildpackage -S -sa -d -uc -us -ui

        # --- 5. Copy results back ---
        cp "$ORIG" /workspace/
        cp "$TMP"/mx3_'"$VERSION"'-1*.dsc /workspace/ 2>/dev/null || true
        cp "$TMP"/mx3_'"$VERSION"'-1*.debian.tar.* /workspace/ 2>/dev/null || true
        cp "$TMP"/mx3_'"$VERSION"'-1*_source.changes /workspace/ 2>/dev/null || true

        echo "---"
        echo "Done. Artifacts in /workspace:"
        ls -1 /workspace/mx3_'"$VERSION"'* 2>/dev/null || true
    '

echo "---"
echo "Artifacts are in: $PARENT_DIR"
ls -1 "$PARENT_DIR"/mx3_${VERSION}* 2>/dev/null || true
