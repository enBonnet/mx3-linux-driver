#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"

COPR_CONFIG="${COPR_CONFIG:-$HOME/.config/copr}"

echo "=== Deploying mx3 $VERSION to COPR ==="

bash "$SCRIPT_DIR/build.sh"

SRPM_FILE="$(ls "$SCRIPT_DIR/output"/SRPMS/mx3-*.src.rpm 2>/dev/null | head -1)"

if [ -z "$SRPM_FILE" ]; then
    echo "ERROR: No SRPM found in $SCRIPT_DIR/output/SRPMS/" >&2
    echo "Did the build succeed?" >&2
    exit 1
fi

echo "Uploading: $(basename "$SRPM_FILE")"

COPR_MOUNT="$(mktemp -d ${TMPDIR:-/tmp}/copr_config.XXXXXX)"
mkdir -p "$COPR_MOUNT/copr"
cp -a "$COPR_CONFIG"/* "$COPR_MOUNT/copr/" 2>/dev/null || true

podman run --rm --entrypoint "" \
    -v "$SCRIPT_DIR/output:/artifacts:Z" \
    -v "$COPR_MOUNT/copr:/root/.config/copr:Z" \
    mx3-copr \
    /bin/sh -c "copr-cli build enbonnet/mx3 /artifacts/SRPMS/$(basename "$SRPM_FILE")"

rm -rf "$COPR_MOUNT"

echo "=== COPR deploy complete ==="