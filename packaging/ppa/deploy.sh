#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"
PARENT_DIR="$(dirname "$PROJECT_DIR")"

GPG_DIR="${GPG_DIR:-$HOME/.gnupg}"
DPUT_CF="${DPUT_CF:-}"

echo "=== Deploying mx3 $VERSION to PPA ==="

bash "$SCRIPT_DIR/build.sh"

CHANGES_FILE="$(ls "$PARENT_DIR"/mx3_${VERSION}-1*_source.changes 2>/dev/null | head -1)"

if [ -z "$CHANGES_FILE" ]; then
    echo "ERROR: No .changes file found in $PARENT_DIR" >&2
    echo "Did the build succeed?" >&2
    exit 1
fi

echo "Signing and uploading: $(basename "$CHANGES_FILE")"

GPG_MOUNT="$(mktemp -d ${TMPDIR:-/tmp}/ppa_gpg.XXXXXX)"
cp -a "$GPG_DIR"/* "$GPG_MOUNT/" 2>/dev/null || true

podman run --rm --entrypoint "" \
    -v "$PARENT_DIR:/workspace:Z" \
    -v "$GPG_MOUNT:/root/.gnupg:Z" \
    ${DPUT_CF:+-v "$DPUT_CF:/root/.dput.cf:Z"} \
    mx3-ppa \
    /bin/sh -c "
        set -euo pipefail
        GPG_KEY=\${GPG_KEY:-}
        if [ -n \"\$GPG_KEY\" ]; then
            gpg --import /root/.gnupg/*.key 2>/dev/null || true
        fi
        debsign -k \$GPG_KEY /workspace/$(basename "$CHANGES_FILE") 2>/dev/null || \
            debsign /workspace/$(basename "$CHANGES_FILE")
        dput ppa:enbonnet/mx3 /workspace/$(basename "$CHANGES_FILE")
    "

rm -rf "$GPG_MOUNT"

echo "=== PPA upload complete ==="