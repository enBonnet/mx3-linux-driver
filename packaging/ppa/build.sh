#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"
PARENT_DIR="$(dirname "$PROJECT_DIR")"

echo "=== Building PPA source package for mx3 $VERSION ==="

podman build -t mx3-ppa "$SCRIPT_DIR"

podman run --rm --entrypoint "" \
    -v "$PROJECT_DIR:/src:Z" \
    -v "$PARENT_DIR:/workspace:Z" \
    mx3-ppa \
    /bin/sh -c "
        set -euo pipefail
        SRC='/src'
        TMP='/tmp'
        PKG='mx3-${VERSION}'
        ORIG=\"\$TMP/mx3_${VERSION}.orig.tar.gz\"

        rm -rf \"\$TMP/\$PKG\" \"\$TMP/\${PKG}-build\" \"\$ORIG\"

        cp -a \"\$SRC\" \"\$TMP/\$PKG\"
        cd \"\$TMP/\$PKG\"
        rm -rf .git debian dist mx3 src/*.o tests/build scripts/build_debian_source_for_ppa.sh scripts/build_debian_in_container.sh
        cd \"\$TMP\"
        tar czf \"\$ORIG\" \"\$PKG\"

        cp -a \"\$SRC\" \"\$TMP/\${PKG}-build\"
        cd \"\$TMP/\${PKG}-build\"
        rm -rf .git dist mx3 src/*.o tests/build scripts/build_debian_source_for_ppa.sh scripts/build_debian_in_container.sh

        cd \"\$TMP/\${PKG}-build\"
        dpkg-buildpackage -S -sa -d -uc -us -ui

        cp \"\$ORIG\" /workspace/
        cp /tmp/mx3_${VERSION}-1*.dsc /workspace/ 2>/dev/null || true
        cp /tmp/mx3_${VERSION}-1*.debian.tar.* /workspace/ 2>/dev/null || true
        cp /tmp/mx3_${VERSION}-1*_source.changes /workspace/ 2>/dev/null || true
        cp /tmp/mx3_${VERSION}-1*_source.buildinfo /workspace/ 2>/dev/null || true

        echo '---'
        echo 'Done. Artifacts in /workspace:'
        ls -1 /workspace/mx3_${VERSION}* 2>/dev/null || true
    "

echo ""
echo "=== PPA artifacts in $PARENT_DIR ==="
ls -1 "$PARENT_DIR"/mx3_${VERSION}* 2>/dev/null || true