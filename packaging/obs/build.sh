#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"
DIST_DIR="$PROJECT_DIR/dist"

echo "=== Building OBS package for mx3 $VERSION ==="

mkdir -p "$SCRIPT_DIR/output"

make -C "$PROJECT_DIR" release-artifacts

podman build -t mx3-obs "$SCRIPT_DIR"

echo ""
echo "OBS artifacts ready in $DIST_DIR"
echo ""
echo "To upload to OBS (Open Build Service):"
echo "  ./packaging/obs/deploy.sh"
echo ""
echo "Or manually:"
echo "  1. osc checkout home:enbonnet/mx3"
echo "  2. Copy the tarball from $DIST_DIR into the checkout"
echo "  3. Update mx3.spec in the checkout if needed"
echo "  4. osc addremove"
echo "  5. osc commit -m 'Update to $VERSION'"
echo ""
echo "Or use the container interactively:"
echo "  podman run --rm -it --entrypoint '' -v \$HOME/.osc:/root/.osc mx3-obs /bin/sh"