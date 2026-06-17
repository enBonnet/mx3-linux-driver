#!/bin/bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$PROJECT_DIR/.."
CONTAINER_IMAGE="debian:trixie-slim"

echo "Pulling Debian container..."
podman pull "$CONTAINER_IMAGE"

echo "Building Debian source package inside container..."
podman run --rm \
    -v "$PROJECT_DIR:/src:Z" \
    -v "$OUT_DIR:/out:Z" \
    "$CONTAINER_IMAGE" \
    bash -c '
        set -euo pipefail
        apt-get update -qq
        apt-get install -y -qq build-essential devscripts debhelper-compat
        cd /src
        # native package: skip orig tarball requirement
        dpkg-buildpackage -S -d -uc -us -ui
        cp /src/../mx3_*.dsc /src/../mx3_*.tar.* /src/../mx3_*.changes /out/
        echo "Done. Artifacts copied to /out/"
    '

echo "---"
echo "Debian source package built in parent directory:"
ls -1 "$OUT_DIR"/mx3_1.*.* 2>/dev/null || true
