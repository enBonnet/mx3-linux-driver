#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="${1:-$PROJECT_DIR/dist}"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"
ARCHIVE_ROOT="MX3-Linux-Driver-${VERSION}"
ARCHIVE_PATH="$OUT_DIR/${ARCHIVE_ROOT}.tar.gz"
CHECKSUM_PATH="$OUT_DIR/${ARCHIVE_ROOT}.sha256"

mkdir -p "$OUT_DIR"

TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

mkdir -p "$TMPDIR/$ARCHIVE_ROOT"

cp -R \
    "$PROJECT_DIR/AGENTS.md" \
    "$PROJECT_DIR/config" \
    "$PROJECT_DIR/debian" \
    "$PROJECT_DIR/deploy" \
    "$PROJECT_DIR/docs" \
    "$PROJECT_DIR/include" \
    "$PROJECT_DIR/LICENSE" \
    "$PROJECT_DIR/Makefile" \
    "$PROJECT_DIR/mx3.spec" \
    "$PROJECT_DIR/PKGBUILD" \
    "$PROJECT_DIR/README.md" \
    "$PROJECT_DIR/src" \
    "$PROJECT_DIR/tests" \
    "$PROJECT_DIR/VERSION" \
    "$PROJECT_DIR/scripts" \
    "$TMPDIR/$ARCHIVE_ROOT"

tar -C "$TMPDIR" -czf "$ARCHIVE_PATH" "$ARCHIVE_ROOT"
sha256sum "$ARCHIVE_PATH" > "$CHECKSUM_PATH"

printf 'build_release_artifacts: wrote %s and %s\n' "$ARCHIVE_PATH" "$CHECKSUM_PATH"
