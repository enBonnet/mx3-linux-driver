#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

fail() {
    printf 'validate_packaging_metadata: %s\n' "$1" >&2
    exit 1
}

grep -q '^Architecture: any$' "$PROJECT_DIR/debian/control" || fail "debian/control must use Architecture: any"
grep -q 'debhelper-compat (= 13)' "$PROJECT_DIR/debian/control" || fail "debian/control must declare debhelper-compat (= 13)"
! grep -q 'dh-sequence-systemd' "$PROJECT_DIR/debian/control" || fail "debian/control must not declare obsolete dh-sequence-systemd"
! grep -q 'systemctl' "$PROJECT_DIR/debian/mx3.postinst" || fail "debian/mx3.postinst should not call systemctl directly"
! grep -q 'systemctl' "$PROJECT_DIR/debian/mx3.postrm" || fail "debian/mx3.postrm should not call systemctl directly"
grep -q "^sha256sums=('TO_BE_FILLED_IN_FOR_RELEASE')$" "$PROJECT_DIR/PKGBUILD" || fail "PKGBUILD checksum placeholder should require release-time update"
grep -q '^check() {$' "$PROJECT_DIR/PKGBUILD" || fail "PKGBUILD must provide check()"

printf 'validate_packaging_metadata: ok\n'
