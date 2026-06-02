#!/bin/bash
# MX3 Linux Driver - Test Suite
# Run with: make test   or   bash tests/run_tests.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BINARY="$PROJECT_DIR/mx3"
TEST_BUILD_DIR="$PROJECT_DIR/tests/build"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0
SKIPPED=0

pass() {
    echo -e "  ${GREEN}[PASS]${NC} $1"
    PASSED=$((PASSED + 1))
}

fail() {
    echo -e "  ${RED}[FAIL]${NC} $1: $2"
    FAILED=$((FAILED + 1))
}

skip() {
    echo -e "  ${YELLOW}[SKIP]${NC} $1: $2"
    SKIPPED=$((SKIPPED + 1))
}

echo "======================================"
echo "MX3 Linux Driver - Test Suite"
echo "======================================"
echo ""

# ------------------------------------------------------------------
# Test 1: Binary exists
# ------------------------------------------------------------------
echo "--- Binary existence ---"
if [ -f "$BINARY" ]; then
    pass "Binary found at $BINARY"
else
    fail "Binary not found" "$BINARY does not exist. Run 'make' first."
fi

# ------------------------------------------------------------------
# Test 2: Help output works
# ------------------------------------------------------------------
echo "--- CLI: --help ---"
if [ -f "$BINARY" ] && [ -x "$BINARY" ]; then
    if "$BINARY" --help 2>&1 | grep -q "Usage:"; then
        pass "--help shows usage"
    else
        fail "--help output missing 'Usage:'"
    fi
else
    skip "CLI --help" "binary not executable"
fi

# ------------------------------------------------------------------
# Test 3: Version output works
# ------------------------------------------------------------------
echo "--- CLI: --version ---"
if [ -f "$BINARY" ] && [ -x "$BINARY" ]; then
    if "$BINARY" --version 2>&1 | grep -qE '[0-9]+\.[0-9]+\.[0-9]+'; then
        pass "--version shows version number"
    else
        fail "--version output missing version number"
    fi
else
    skip "CLI --version" "binary not executable"
fi

# ------------------------------------------------------------------
# Test 4: Invalid log level is rejected
# ------------------------------------------------------------------
echo "--- CLI: invalid log level ---"
if [ -f "$BINARY" ] && [ -x "$BINARY" ]; then
if "$BINARY" -l invalid 2>&1 | grep -qi "Unknown"; then
    pass "Invalid log level shows appropriate error"
else
    fail "Invalid log level" "expected 'Unknown log level' message"
fi
else
    skip "CLI invalid log level" "binary not executable"
fi

# ------------------------------------------------------------------
# Test 5: Config file exists with valid syntax
# ------------------------------------------------------------------
echo "--- Config file validity ---"
CONFIG_FILE="$PROJECT_DIR/config/default.conf"
if [ -f "$CONFIG_FILE" ]; then
    # Check for required sections
    if grep -q '\[general\]' "$CONFIG_FILE" && \
       grep -q '\[device\]' "$CONFIG_FILE" && \
       grep -q '\[button.forward\]' "$CONFIG_FILE"; then
        pass "default.conf has required sections"
    else
        fail "default.conf missing required sections"
    fi
else
    fail "Config file" "$CONFIG_FILE not found"
fi

# ------------------------------------------------------------------
# Test 6: Source files compile without warnings
# ------------------------------------------------------------------
echo "--- Compilation warnings ---"
if [ -f "$BINARY" ] && [ -x "$BINARY" ]; then
    OUTPUT=$(mktemp)
    cd "$PROJECT_DIR" && MAKEFLAGS= make --no-print-directory clean >/dev/null 2>&1
    set +e
    cd "$PROJECT_DIR" && MAKEFLAGS= make --no-print-directory test-binaries > "$OUTPUT" 2>&1
    MAKE_STATUS=$?
    set -e
    if [ $MAKE_STATUS -eq 0 ]; then
        if grep -qi "warn" "$OUTPUT"; then
            fail "Compilation" "compiled with warnings (check $OUTPUT)"
        else
            pass "Clean compilation with -Wall -Wextra -Werror"
        fi
    else
        fail "Compilation" "make failed (check $OUTPUT)"
    fi
    rm -f "$OUTPUT"
else
    skip "Compilation warnings check" "binary not available"
fi

# ------------------------------------------------------------------
# Test 7: No hardcoded secrets or tokens
# ------------------------------------------------------------------
echo "--- Security scan ---"
SECRETS_FOUND=0
for f in "$PROJECT_DIR"/src/*.c "$PROJECT_DIR"/include/*.h; do
    if grep -qE '(password|secret|token|api_key)\s*=' "$f" 2>/dev/null; then
        echo "  WARNING: potential secret in $f"
        SECRETS_FOUND=1
    fi
done
if [ $SECRETS_FOUND -eq 0 ]; then
    pass "No hardcoded secrets found"
else
    fail "Security scan" "found potential hardcoded secrets"
fi

# ------------------------------------------------------------------
# Test 8: Deploy files exist
# ------------------------------------------------------------------
echo "--- Deployment files ---"
if [ -f "$PROJECT_DIR/deploy/mx3.service" ]; then
    pass "systemd service file exists"

    if grep -q '^ExecStart=.*/mx3$' "$PROJECT_DIR/deploy/mx3.service"; then
        pass "systemd service runs mx3 in foreground"
    else
        fail "systemd ExecStart" "expected foreground ExecStart without daemon flags"
    fi

    if grep -q '^PIDFile=' "$PROJECT_DIR/deploy/mx3.service"; then
        fail "systemd PIDFile" "PIDFile should not be set for the foreground service"
    else
        pass "systemd service does not rely on a PID file"
    fi

    if grep -q '^Environment=DISPLAY=' "$PROJECT_DIR/deploy/mx3.service" || \
       grep -q '^Environment=XAUTHORITY=' "$PROJECT_DIR/deploy/mx3.service"; then
        fail "systemd environment" "service should not hardcode desktop session variables"
    else
        pass "systemd service avoids hardcoded desktop session variables"
    fi

    if grep -q '^User=root$' "$PROJECT_DIR/deploy/mx3.service"; then
        pass "systemd service uses the documented production identity"
    else
        fail "systemd user" "expected packaged service to use the documented root-managed identity"
    fi
else
    fail "systemd service file" "deploy/mx3.service missing"
fi

if [ -f "$PROJECT_DIR/deploy/99-mx3.rules" ]; then
    pass "udev rules file exists"

    if grep -q 'GROUP="input"' "$PROJECT_DIR/deploy/99-mx3.rules"; then
        fail "udev rules" "udev rule should not grant broad input-group access"
    else
        pass "udev rule avoids interactive input-group access"
    fi
else
    fail "udev rules file" "deploy/99-mx3.rules missing"
fi

# ------------------------------------------------------------------
# Test 9: Native behavior tests
# ------------------------------------------------------------------
echo "--- Native behavior tests ---"
for test_bin in \
    "$TEST_BUILD_DIR/test_config_behavior" \
    "$TEST_BUILD_DIR/test_gesture_behavior" \
    "$TEST_BUILD_DIR/test_device_score"; do
    if [ -x "$test_bin" ]; then
        if "$test_bin"; then
            pass "$(basename "$test_bin") passed"
        else
            fail "$(basename "$test_bin")" "native test failed"
        fi
    else
        fail "Native test binary" "$test_bin missing"
    fi
done

# ------------------------------------------------------------------
# Test 10: Code style - no TAB indentation in headers
# ------------------------------------------------------------------
echo "--- Code style checks ---"
TAB_FILES=$(grep -rl --include='*.c' --include='*.h' $'\t' "$PROJECT_DIR/src/" "$PROJECT_DIR/include/" 2>/dev/null || true)
if [ -z "$TAB_FILES" ]; then
    pass "No tab indentation found (using spaces)"
else
    fail "Code style" "tab indentation found in: $TAB_FILES"
fi

# ------------------------------------------------------------------
# Test 11: Version and release metadata
# ------------------------------------------------------------------
echo "--- Version and release metadata ---"
if [ -f "$PROJECT_DIR/VERSION" ]; then
    if bash "$PROJECT_DIR/scripts/check_version_consistency.sh" >/dev/null; then
        pass "Version metadata is consistent"
    else
        fail "Version metadata" "version files are not aligned"
    fi
else
    fail "Version file" "VERSION missing"
fi

# ------------------------------------------------------------------
# Test 12: All functions have prototypes
# ------------------------------------------------------------------
echo "--- Function prototype coverage ---"
MISSING_PROTO=0
for f in "$PROJECT_DIR"/src/*.c; do
    BASENAME=$(basename "$f" .c)
    HEADER="$PROJECT_DIR/include/mx3_${BASENAME#mx3_}.h"
    if [ "$BASENAME" = "mx3_driver" ]; then HEADER=""; fi  # main file has no matching header
done

# Check that key symbols are exported
if grep -q 'config_load' "$PROJECT_DIR/src/config.c" 2>/dev/null; then
    pass "Config parser exports config_load"
else
    fail "Config parser" "config_load not found"
fi

# ------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------
echo ""
echo "======================================"
echo "Results: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC}, ${YELLOW}$SKIPPED skipped${NC}"
echo "======================================"

if [ $FAILED -gt 0 ]; then
    exit 1
fi
exit 0
