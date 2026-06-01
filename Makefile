# MX3 Linux Driver - Makefile
# Build system for the gesture-based keyboard shortcut mapper

CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -O2 -std=gnu11 -D_GNU_SOURCE
LDFLAGS :=

TARGET  := mx3

SRC_DIR := src
INC_DIR := include

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(SOURCES:.c=.o)

PREFIX      ?= /usr/local
BIN_DIR     ?= $(PREFIX)/bin
CONFIG_DIR  ?= /etc/mx3
SYSTEMD_DIR ?= $(PREFIX)/lib/systemd/system
UDEV_DIR    ?= /etc/udev/rules.d
DOC_DIR     ?= $(PREFIX)/share/doc/mx3
LICENSE_DIR ?= $(PREFIX)/share/licenses/mx3

CFLAGS += -I$(INC_DIR)

.PHONY: all clean install install-local uninstall test lint format debug

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(INC_DIR)/*.h)
	$(CC) $(CFLAGS) -c -o $@ $<

# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------
test: $(TARGET)
	@echo "=== Running test suite ==="
	@cd tests && bash run_tests.sh
	@echo "=== Tests complete ==="

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
debug: CFLAGS = -Wall -Wextra -g -O0 -DDEBUG -std=c11 -I$(INC_DIR) -fsanitize=address -fsanitize=undefined
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
	@echo "Permissions (one-time setup, then log out and back in):"
	@echo "  sudo modprobe uinput"
	@echo "  sudo usermod -aG input \$$USER"
	@echo ""
	@echo "To reload udev rules:"
	@echo "  sudo udevadm control --reload-rules && sudo udevadm trigger"
	@echo ""
	@echo "To enable automatic startup:"
	@echo "  sudo systemctl daemon-reload"
	@echo "  sudo systemctl enable --now mx3"

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
	@echo "Permissions (one-time setup, then log out and back in):"
	@echo "  sudo modprobe uinput"
	@echo "  sudo usermod -aG input \$$USER"
	@echo ""
	@echo "Until you log out and back in, use:"
	@echo "  sg input -c mx3"

uninstall:
	rm -f $(DESTDIR)$(BIN_DIR)/$(TARGET)
	rm -rf $(DESTDIR)$(CONFIG_DIR)
	rm -f $(DESTDIR)$(SYSTEMD_DIR)/mx3.service
	rm -f $(DESTDIR)$(UDEV_DIR)/99-mx3.rules
	rm -rf $(DESTDIR)$(DOC_DIR)
	rm -rf $(DESTDIR)$(LICENSE_DIR)
	@echo "Uninstall complete."

# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------
clean:
	rm -f $(OBJECTS) $(TARGET)
