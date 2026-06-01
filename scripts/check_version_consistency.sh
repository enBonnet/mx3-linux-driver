#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$(tr -d '\n' < "$PROJECT_DIR/VERSION")"

fail() {
    printf 'check_version_consistency: %s\n' "$1" >&2
    exit 1
}

grep -q "^pkgver=${VERSION}$" "$PROJECT_DIR/PKGBUILD" || fail "PKGBUILD pkgver does not match VERSION"
grep -q "^Version:        ${VERSION}$" "$PROJECT_DIR/mx3.spec" || fail "mx3.spec Version does not match VERSION"
grep -q "^mx3 (${VERSION}-1)" "$PROJECT_DIR/debian/changelog" || fail "debian/changelog version does not match VERSION"

printf 'check_version_consistency: ok (%s)\n' "$VERSION"
