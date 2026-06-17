#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"
DIST_DIR="$PROJECT_DIR/dist"

OSC_RC="${OSC_RC:-$HOME/.oscrc}"

echo "=== Deploying mx3 $VERSION to OBS ==="

bash "$SCRIPT_DIR/build.sh"

if [ ! -f "$OSC_RC" ]; then
    echo "ERROR: No .oscrc found at $OSC_RC" >&2
    echo "Create one with: cat > ~/.oscrc << 'EOF'" >&2
    echo "[general]" >&2
    echo "apiurl = https://api.opensuse.org" >&2
    echo "" >&2
    echo "[https://api.opensuse.org]" >&2
    echo "user = YOUR_USERNAME" >&2
    echo "pass = YOUR_PASSWORD" >&2
    echo "EOF" >&2
    exit 1
fi

make -C "$PROJECT_DIR" release-artifacts

TARBALL="$(ls "$DIST_DIR"/MX3-Linux-Driver-${VERSION}.tar.gz 2>/dev/null | head -1)"

if [ -z "$TARBALL" ]; then
    echo "ERROR: No tarball found in $DIST_DIR" >&2
    exit 1
fi

echo "Uploading to OBS home:enbonnet/mx3..."

OSC_MOUNT="$(mktemp -d ${TMPDIR:-/tmp}/obs_config.XXXXXX)"
cp -a "$OSC_RC" "$OSC_MOUNT/.oscrc"

podman run --rm --entrypoint "" \
    -v "$PROJECT_DIR:/src:Z" \
    -v "$DIST_DIR:/dist:Z" \
    -v "$OSC_MOUNT/.oscrc:/root/.oscrc:Z" \
    mx3-obs \
    /bin/sh -c "
        set -euo pipefail
        TMPDIR=\$(mktemp -d)
        cd \"\$TMPDIR\"
        osc checkout home:enbonnet mx3
        cd home:enbonnet/mx3
        osc service localrun || true
        cp /dist/$(basename "$TARBALL") .
        cp /src/mx3.spec .
        cp /src/obs/_service . 2>/dev/null || true
        osc addremove
        osc commit -m 'Update to $VERSION'
    "

rm -rf "$OSC_MOUNT"

echo "=== OBS deploy complete ==="