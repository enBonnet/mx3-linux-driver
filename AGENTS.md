# AGENTS.md — MX3 Linux Driver

## Build & test

```bash
make              # release build (-O2, -Wall -Wextra -Werror, no warnings allowed)
make debug        # debug build with -O0 -g -fsanitize=address,undefined
make test         # run shell-based test suite (requires binary already built)
make clean        # remove .o and mx3 binary
```

Test suite (`tests/run_tests.sh`) checks binary existence, CLI flags, config validity, compilation warnings, file presence, and code style. No C unit tests exist.

## Architecture

- **Single binary** `mx3` — no shared libraries, no sub-packages
- **Entry point**: `src/mx3_driver.c:89` (`main()`) — CLI parsing, device open, main event loop
- **Source modules** (all in `src/`, headers in `include/`):
  - `config.c` — INI parser, key name→code mapping, defaults builder
  - `device.c` — scans `/dev/input/event*` for matching mouse (scoring: VID/PID=100, name=50, exact=+25)
  - `gesture.c` — button-press tracking, motion accumulation, direction classification
  - `uinput.c` — creates/destroys virtual keyboard via `/dev/uinput`, injects key combos
  - `logging.c` — timestamped logging to stderr
- **`include/mx3_common.h`** is the master header. It includes `<linux/input.h>` and `<linux/uinput.h>` and declares all types, enums, and function prototypes. Every `.c` includes only `mx3_common.h` plus its own subsystem header.

## Key conventions

- **C11 with GNU extensions** (`-std=gnu11`)
- **Spaces only** — tab indentation is a test failure
- **No unused variables or implicit casts** — `-Wall -Wextra -Werror` enforced
- **All structs are `typedef`'d** with `_t` suffix (e.g. `config_t`, `gesture_tracker_t`)
- **Logging uses macros**: `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR` — defined in `mx3_common.h`
- **Config key names must match** the `key_map[]` table in `config.c:29` exactly (case-insensitive via `strcasecmp`)

## Config file search order

Config is loaded from the first found:
1. `./mx3.conf`
2. `~/.config/mx3/config.conf`
3. `~/.config/mx3.conf`
4. `/etc/mx3/config.conf`

If none found, built-in defaults from `config_set_defaults()` in `config.c` are used.

## Hardware requirements

The binary reads raw `struct input_event` from an evdev fd and injects keys via `/dev/uinput`. It will not produce meaningful output without a connected mouse and `uinput` loaded. The test suite can run without hardware.

## Permissions

User must be in `input` group and `uinput` kernel module must be loaded:
```bash
sudo modprobe uinput
sudo usermod -aG input $USER
```

## Packaging

Platform packages (`debian/`, `PKGBUILD`, `mx3.spec`) are thin wrappers that call `make PREFIX=/usr install`. The `@PREFIX@` placeholder in `deploy/mx3.service` is `sed`-substituted during install.
