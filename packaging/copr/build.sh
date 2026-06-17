#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"
DIST_DIR="$PROJECT_DIR/dist"

echo "=== Building COPR SRPM for mx3 $VERSION ==="

mkdir -p "$SCRIPT_DIR/output"

make -C "$PROJECT_DIR" release-artifacts

TARBALL="$DIST_DIR/MX3-Linux-Driver-${VERSION}.tar.gz"
cp "$PROJECT_DIR/mx3.spec" "$DIST_DIR/"

cp "$TARBALL" "$DIST_DIR/mx3-${VERSION}.tar.gz"

podman build -t mx3-copr "$SCRIPT_DIR"

podman run --rm --entrypoint "" \
    -v "$DIST_DIR:/src:Z" \
    -v "$SCRIPT_DIR/output:/output:Z" \
    mx3-copr \
    /bin/sh -c "
        set -euo pipefail
        mkdir -p /output/SOURCES /output/SRPMS
        rpmbuild -bs \
            --define '_topdir /output' \
            --define '_sourcedir /src' \
            /src/mx3.spec
    "

echo ""
echo "=== COPR SRPM in $SCRIPT_DIR/output ==="
ls -1 "$SCRIPT_DIR/output"/SRPMS/*.src.rpm 2>/dev/null || true
echo ""
echo "To upload to COPR:"
echo "  copr-cli build enbonnet/mx3 $SCRIPT_DIR/output/SRPMS/mx3-*.src.rpm"