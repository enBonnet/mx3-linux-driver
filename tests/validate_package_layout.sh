#!/bin/bash

set -euo pipefail

STAGING_ROOT="${1:?usage: validate_package_layout.sh <staging-root>}"

SERVICE_FILE="$STAGING_ROOT/usr/lib/systemd/system/mx3.service"
CONFIG_FILE="$STAGING_ROOT/etc/mx3/config.conf"
UDEV_RULE="$STAGING_ROOT/etc/udev/rules.d/99-mx3.rules"
README_FILE="$STAGING_ROOT/usr/share/doc/mx3/README.md"
LICENSE_FILE="$STAGING_ROOT/usr/share/licenses/mx3/LICENSE"
BINARY_FILE="$STAGING_ROOT/usr/bin/mx3"

fail() {
    printf 'validate_package_layout: %s\n' "$1" >&2
    exit 1
}

[ -x "$BINARY_FILE" ] || fail "missing staged binary"
[ -f "$CONFIG_FILE" ] || fail "missing staged config file"
[ -f "$SERVICE_FILE" ] || fail "missing staged systemd unit"
[ -f "$UDEV_RULE" ] || fail "missing staged udev rule"
[ -f "$README_FILE" ] || fail "missing staged README"
[ -f "$LICENSE_FILE" ] || fail "missing staged LICENSE"

grep -q '^ExecStart=/usr/bin/mx3$' "$SERVICE_FILE" || fail "service should run mx3 in foreground"
! grep -q '^PIDFile=' "$SERVICE_FILE" || fail "service should not install a PID file"
! grep -q '^Environment=DISPLAY=' "$SERVICE_FILE" || fail "service should not hardcode DISPLAY"
! grep -q '^Environment=XAUTHORITY=' "$SERVICE_FILE" || fail "service should not hardcode XAUTHORITY"
grep -q '^User=root$' "$SERVICE_FILE" || fail "service should run as root-managed production identity"
! grep -q 'GROUP="input"' "$UDEV_RULE" || fail "udev rule should not grant input-group access"

printf 'validate_package_layout: ok\n'
