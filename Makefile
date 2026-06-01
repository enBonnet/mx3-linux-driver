# MX3 Linux Driver - Makefile
# Build system for the gesture-based keyboard shortcut mapper

CC      := gcc
VERSION := $(shell tr -d '\n' < VERSION)
CFLAGS  := -Wall -Wextra -Werror -O2 -std=gnu11 -D_GNU_SOURCE -DMX3_VERSION=\"$(VERSION)\"
LDFLAGS :=

TARGET  := mx3

SRC_DIR := src
INC_DIR := include
TEST_DIR := tests
TEST_BUILD_DIR := $(TEST_DIR)/build

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(SOURCES:.c=.o)
TEST_BINARIES := $(TEST_BUILD_DIR)/test_config_behavior $(TEST_BUILD_DIR)/test_gesture_behavior $(TEST_BUILD_DIR)/test_device_score

PREFIX      ?= /usr/local
BIN_DIR     ?= $(PREFIX)/bin
CONFIG_DIR  ?= /etc/mx3
SYSTEMD_DIR ?= $(PREFIX)/lib/systemd/system
UDEV_DIR    ?= /etc/udev/rules.d
DOC_DIR     ?= $(PREFIX)/share/doc/mx3
LICENSE_DIR ?= $(PREFIX)/share/licenses/mx3

CFLAGS += -I$(INC_DIR)

.PHONY: all clean install install-local uninstall uninstall-local test test-binaries lint format debug validate-package-layout validate-packaging release-artifacts verify-version

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(INC_DIR)/*.h)
	$(CC) $(CFLAGS) -c -o $@ $<

# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------
test-binaries: $(TEST_BINARIES)

test: $(TARGET) test-binaries
	@echo "=== Running test suite ==="
	@cd tests && bash run_tests.sh
	@echo "=== Tests complete ==="

$(TEST_BUILD_DIR):
	install -d $(TEST_BUILD_DIR)

$(TEST_BUILD_DIR)/test_config_behavior: $(TEST_DIR)/test_config_behavior.c $(SRC_DIR)/config.c $(SRC_DIR)/logging.c $(wildcard $(INC_DIR)/*.h) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $(TEST_DIR)/test_config_behavior.c $(SRC_DIR)/config.c $(SRC_DIR)/logging.c

$(TEST_BUILD_DIR)/test_gesture_behavior: $(TEST_DIR)/test_gesture_behavior.c $(SRC_DIR)/gesture.c $(SRC_DIR)/logging.c $(wildcard $(INC_DIR)/*.h) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $(TEST_DIR)/test_gesture_behavior.c $(SRC_DIR)/gesture.c $(SRC_DIR)/logging.c

$(TEST_BUILD_DIR)/test_device_score: $(TEST_DIR)/test_device_score.c $(SRC_DIR)/device.c $(SRC_DIR)/logging.c $(wildcard $(INC_DIR)/*.h) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $(TEST_DIR)/test_device_score.c $(SRC_DIR)/device.c $(SRC_DIR)/logging.c

validate-package-layout: $(TARGET)
	@TMPDIR=$$(mktemp -d) && \
	trap 'rm -rf "$$TMPDIR"' EXIT && \
	$(MAKE) DESTDIR="$$TMPDIR" PREFIX=/usr install >/dev/null && \
	bash tests/validate_package_layout.sh "$$TMPDIR"

validate-packaging: validate-package-layout
	@bash tests/validate_packaging_metadata.sh

verify-version:
	@bash scripts/check_version_consistency.sh

release-artifacts: verify-version
	@bash scripts/build_release_artifacts.sh dist

# ---------------------------------------------------------------------------
# Code quality
# ---------------------------------------------------------------------------
lint:
	@command -v cppcheck >/dev/null 2>&1 && cppcheck --enable=all --inconclusive --std=c11 --suppress=missingIncludeSystem $(SRC_DIR)/*.c $(INC_DIR)/*.h || echo "cppcheck not installed, skipping"
	@command -v clang-tidy >/dev/null 2>&1 && clang-tidy -p . $(SRC_DIR)/*.c -- -I$(INC_DIR) || echo "clang-tidy not installed, skipping"

format:
	@command -v clang-format >/dev/null 2>&1 && clang-format -i $(SRC_DIR)/*.c $(INC_DIR)/*.h || echo "clang-format not installed, skipping"

# ---------------------------------------------------------------------------
# Debug build
# ---------------------------------------------------------------------------
debug: CFLAGS = -Wall -Wextra -g -O0 -DDEBUG -std=gnu11 -D_GNU_SOURCE -DMX3_VERSION=\"$(VERSION)\" -I$(INC_DIR) -fsanitize=address -fsanitize=undefined
debug: clean $(TARGET)

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------
install: $(TARGET)
	install -d $(DESTDIR)$(BIN_DIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BIN_DIR)/$(TARGET)

	install -d $(DESTDIR)$(CONFIG_DIR)
	install -m 644 config/default.conf $(DESTDIR)$(CONFIG_DIR)/config.conf

	install -d $(DESTDIR)$(SYSTEMD_DIR)
	sed 's|@PREFIX@|$(PREFIX)|g' deploy/mx3.service > $(DESTDIR)$(SYSTEMD_DIR)/mx3.service
	chmod 644 $(DESTDIR)$(SYSTEMD_DIR)/mx3.service

	install -d $(DESTDIR)$(UDEV_DIR)
	install -m 644 deploy/99-mx3.rules $(DESTDIR)$(UDEV_DIR)/99-mx3.rules

	install -d $(DESTDIR)$(DOC_DIR)
	install -m 644 README.md $(DESTDIR)$(DOC_DIR)/README.md

	install -d $(DESTDIR)$(LICENSE_DIR)
	install -m 644 LICENSE $(DESTDIR)$(LICENSE_DIR)/LICENSE

	@echo ""
	@echo "Installation complete ($(BIN_DIR)/$(TARGET))."
	@echo ""
	@echo "Production runtime (recommended):"
	@echo "  sudo modprobe uinput"
	@echo "  sudo udevadm control --reload-rules && sudo udevadm trigger"
	@echo "  sudo systemctl daemon-reload"
	@echo "  sudo systemctl enable --now mx3"
	@echo "  sudo systemctl status mx3"
	@echo "  sudo journalctl -u mx3 -f"
	@echo ""
	@echo "Note: running 'mx3' directly as a regular user may fail with /dev/uinput permission denied."
	@echo "For foreground debugging, use: sudo mx3 -l debug"

install-local: $(TARGET)
	install -d $(HOME)/.local/bin
	install -m 755 $(TARGET) $(HOME)/.local/bin/$(TARGET)
	install -d $(HOME)/.config/mx3
	install -m 644 config/default.conf $(HOME)/.config/mx3/config.conf
	@echo ""
	@echo "Installed to $(HOME)/.local/bin/$(TARGET)."
	@echo "Add ~/.local/bin to your PATH if it is not already there:"
	@echo "  echo 'export PATH=\"\$$HOME/.local/bin:\$$PATH\"' >> ~/.bashrc"
	@echo ""
	@echo "Manual local runs still require access to /dev/input/event* and /dev/uinput."
	@echo "  sudo modprobe uinput"
	@echo "  sudo $(HOME)/.local/bin/$(TARGET) -l debug"
	@echo ""
	@echo "For production use, prefer the packaged systemd service."

uninstall:
	@if [ -z "$(DESTDIR)" ] && command -v systemctl >/dev/null 2>&1; then \
		echo "Stopping and disabling mx3.service if present..."; \
		systemctl disable --now mx3 >/dev/null 2>&1 || true; \
	fi
	rm -f $(DESTDIR)$(BIN_DIR)/$(TARGET)
	rm -rf $(DESTDIR)$(CONFIG_DIR)
	rm -f $(DESTDIR)$(SYSTEMD_DIR)/mx3.service
	rm -f $(DESTDIR)$(UDEV_DIR)/99-mx3.rules
	rm -rf $(DESTDIR)$(DOC_DIR)
	rm -rf $(DESTDIR)$(LICENSE_DIR)
	@if [ -z "$(DESTDIR)" ] && command -v systemctl >/dev/null 2>&1; then \
		echo "Reloading systemd daemon..."; \
		systemctl daemon-reload >/dev/null 2>&1 || true; \
	fi
	@if [ -z "$(DESTDIR)" ] && command -v udevadm >/dev/null 2>&1; then \
		echo "Reloading udev rules..."; \
		udevadm control --reload-rules >/dev/null 2>&1 || true; \
		udevadm trigger >/dev/null 2>&1 || true; \
	fi
	@echo "Uninstall complete."

uninstall-local:
	rm -f $(HOME)/.local/bin/$(TARGET)
	rm -rf $(HOME)/.config/mx3
	@echo "Local uninstall complete."

# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------
clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_BINARIES)
	rm -rf $(TEST_BUILD_DIR)
