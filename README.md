# MX3 Linux Driver

Gesture remapping driver for Logitech MX Master 3 (and MX2/MX2S/Anywhere 3) mice on Linux. Hold a mouse button, move in a direction, and trigger keyboard shortcuts — workspace switching, volume control, media playback, browser tab navigation, and more.

## Overview

- Detects the MX3 mouse via USB VID/PID and/or device name
- Tracks mouse movement while a button is held
- Classifies gestures: tap, swipe (4 cardinal), diagonal (4 corners)
- Injects keyboard shortcuts via a virtual `/dev/uinput` device
- Fully configurable via an INI config file

## Quick Start

```bash
# Build
make

# Install system-wide (writes to /usr/local and /etc, so root is required)
sudo make install

# Or install to ~/.local/bin (no root needed, add ~/.local/bin to PATH if not already)
make install-local

# Set up the packaged service
sudo modprobe uinput
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo systemctl daemon-reload
sudo systemctl enable --now mx3

# Verify the service is running
sudo systemctl status mx3

# Follow logs while testing gestures
sudo journalctl -u mx3 -f

# Foreground debugging requires device access, so use sudo
sudo mx3 -l debug
```

If you run `mx3` directly as your regular user after the default system install,
it will usually fail with `Cannot open /dev/uinput: Permission denied`. That is
expected: the default install path is the root-managed `systemd` service.

## Project Structure

```
├── Makefile             # Single build & install authority
├── PKGBUILD             # Arch Linux (paru -S mx3)
├── mx3.spec             # Fedora / CentOS / RHEL (COPR + rpmbuild)
├── debian/              # Debian / Ubuntu (apt install mx3)
│   ├── control          #   Package metadata
│   ├── rules            #   dh wrapper that calls make install
│   ├── changelog        #   Version history
│   ├── mx3.postinst     #   Post-install (enable service, reload udev)
│   └── mx3.postrm       #   Post-remove (disable service)
├── deploy/
│   ├── mx3.service      #   systemd unit (runs mx3 in foreground)
│   └── 99-mx3.rules     #   udev rule for the packaged service runtime
├── config/
│   └── default.conf     #   Installed to /etc/mx3/config.conf
├── VERSION              #   Single source of truth for release version
├── scripts/             #   Release and verification helpers
├── src/                 #   Modular C source files
├── include/             #   Header files
└── tests/               #   Shell suite + native behavior tests
```

The `Makefile` is the single authority for installation. Every platform package
(Debian, Arch, RPM) is a thin wrapper that calls `make PREFIX=/usr install`.
The `@PREFIX@` placeholder in `deploy/mx3.service` is patched during install so
the `ExecStart` path matches the chosen prefix (`/usr/bin/mx3` on AUR/RPM/deb,
`/usr/local/bin/mx3` on manual installs). The service runs `mx3` in the foreground
and lets `systemd` manage restart and lifecycle.

## Installation

### Debian / Ubuntu

```bash
# Install build dependencies
sudo apt install build-essential debhelper

# Build the .deb package
dpkg-buildpackage -b -uc -us

# Install
sudo apt install ../mx3_1.0.0-1_amd64.deb
```

The `debian/` directory provides the package metadata. `dpkg-buildpackage` invokes
`make PREFIX=/usr install` under the hood, which installs the binary, config,
systemd service, udev rule, README, and license into the `.deb` file structure.

### Arch Linux

```bash
# Install from AUR
paru -S mx3

# Or build manually from the PKGBUILD
makepkg -si
```

The `PKGBUILD` calls `make PREFIX=/usr install`, relying on the Makefile to handle
all file placement (binary, config, service, udev, docs). The `@PREFIX@` placeholder
in the service file is substituted automatically.

### Fedora (COPR)

```bash
sudo dnf copr enable enbonnet/mx3
sudo dnf install -y mx3
sudo systemctl enable --now mx3
```

The [enbonnet/mx3 COPR](https://copr.fedorainfracloud.org/coprs/enbonnet/mx3/) builds the `mx3.spec` automatically for current Fedora releases.

### CentOS / RHEL / Fedora (manual build)

```bash
# Install build dependencies
sudo dnf install rpm-build rpmdevtools

# Set up the rpmbuild tree
rpmdev-setuptree

# Build the RPM from the spec file
rpmbuild -ba mx3.spec

# Install
sudo rpm -i ~/rpmbuild/RPMS/x86_64/mx3-1.0.1-2.*.rpm
```

The `mx3.spec` file calls `make PREFIX=/usr UDEV_DIR=%{_prefix}/lib/udev/rules.d install`,
placing udev rules in the RPM-standard location while the Makefile defaults to `/etc/udev/rules.d`
for manual and Debian/Arch installs.

### Enable auto-start (all platforms)

```bash
sudo systemctl enable --now mx3
sudo systemctl status mx3
```

Production packages install a root-managed `systemd` service. That is the
supported production runtime because it keeps device access scoped to the
service instead of requiring broad interactive-user access.

## Supported Mice

| Model | VID:PID | Notes |
|-------|---------|-------|
| MX Master 3 | `0x046d:0xc08a` | Primary target |
| MX Master 3S | `0x046d:0x408a` | Bolt receiver |
| MX Master 2S | `0x046d:0x4085` | |
| MX Anywhere 3 | `0x046d:0x4091` | Compact version |
| Generic Logitech | Any `0x046d` device | Falls back to name matching |

## Mapped Buttons

| Button | Linux code | Config section |
|--------|-----------|----------------|
| Forward | `BTN_FORWARD` (0x115) | `[button.forward]` |
| Back | `BTN_BACK` (0x116) | `[button.back]` |
| Middle click | `BTN_MIDDLE` (0x112) | `[button.middle]` |
| Extra (DPI) | `BTN_EXTRA` (0x114) | `[button.extra]` |
| Side scroll click | `BTN_SIDE` (0x117) | `[button.side]` |
| Thumb gesture | `BTN_TASK` (0x11a) | `[button.gesture]` |

## Default Gestures

### Forward button (hold + move)

| Gesture | Action |
|---------|--------|
| Tap | App launcher (`Super`) |
| Swipe left | Previous workspace (`Super + [`) |
| Swipe right | Next workspace (`Super + ]`) |
| Swipe up | Volume up |
| Swipe down | Volume down |

### Back button (hold + move)

| Gesture | Action |
|---------|--------|
| Tap | App launcher (`Super`) |
| Swipe left | Previous tab (`Ctrl + PageUp`) |
| Swipe right | Next tab (`Ctrl + PageDown`) |
| Swipe up | Brightness up |
| Swipe down | Brightness down |

### Middle button

| Gesture | Action |
|---------|--------|
| Tap | Mute toggle |

### Extra button

| Gesture | Action |
|---------|--------|
| Tap | Play/Pause media |
| Swipe left | Previous track |
| Swipe right | Next track |
| Swipe up | Volume up |
| Swipe down | Volume down |

## CLI Options

```
Usage: mx3 [options]

  -c, --config FILE     Use a specific config file
  -d, --device PATH     Force a specific input device (e.g., /dev/input/event5)
  -l, --log-level L     Set log level: debug | info | warn | error
  --daemon              Fork into background
  --pid-file FILE       Write PID to FILE
  -v, --version         Print version
  -h, --help            Show this help
```

## Configuration

Copy the default config and edit it:

```bash
cp /etc/mx3/config.conf ~/.config/mx3/config.conf
# edit ~/.config/mx3/config.conf
```

Config files are searched in this order:

1. `./mx3.conf` (current directory)
2. `~/.config/mx3/config.conf`
3. `~/.config/mx3.conf`
4. `/etc/mx3/config.conf`

### Config file format (INI)

```ini
[general]
tap_timeout = 0.2          # Seconds to distinguish tap from hold
motion_threshold = 50      # Minimum pixels for gesture detection
log_level = info           # debug | info | warn | error

[device]
vid = 0x046d               # USB vendor ID (0 = any)
pid = 0                    # USB product ID (0 = any)
device_name = Logitech     # Substring to match device name

[button.forward]
event_code = 0x115
motion_threshold = 50

action.tap.keys = KEY_LEFTMETA
action.tap.description = Open app launcher

action.left.keys = KEY_LEFTMETA, KEY_LEFTBRACE
action.left.description = Previous workspace

action.right.keys = KEY_LEFTMETA, KEY_RIGHTBRACE
action.right.description = Next workspace

action.up.keys = KEY_VOLUMEUP
action.up.description = Volume up

action.down.keys = KEY_VOLUMEDOWN
action.down.description = Volume down

# Diagonal gestures
action.upleft.keys = KEY_LEFTCTRL, KEY_LEFT
action.upleft.description = Ctrl+Left

action.upright.keys = KEY_LEFTCTRL, KEY_RIGHT
action.upright.description = Ctrl+Right
```

### Available gesture directions

- `tap` — quick press with no movement
- `left`, `right`, `up`, `down` — cardinal swipes
- `upleft`, `upright`, `downleft`, `downright` — diagonal swipes

### Available KEY_* names

| Category | Key names |
|----------|-----------|
| Modifiers | `KEY_LEFTCTRL`, `KEY_RIGHTCTRL`, `KEY_LEFTSHIFT`, `KEY_RIGHTSHIFT`, `KEY_LEFTALT`, `KEY_RIGHTALT`, `KEY_LEFTMETA`, `KEY_RIGHTMETA` |
| Navigation | `KEY_LEFT`, `KEY_RIGHT`, `KEY_UP`, `KEY_DOWN`, `KEY_HOME`, `KEY_END`, `KEY_PAGEUP`, `KEY_PAGEDOWN` |
| Media | `KEY_MUTE`, `KEY_VOLUMEUP`, `KEY_VOLUMEDOWN`, `KEY_PLAYPAUSE`, `KEY_STOPCD`, `KEY_PREVIOUSSONG`, `KEY_NEXTSONG` |
| Brightness | `KEY_BRIGHTNESSUP`, `KEY_BRIGHTNESSDOWN` |
| Function | `KEY_F1` - `KEY_F24` |
| Letters | `KEY_A` - `KEY_Z` |
| Numbers | `KEY_0` - `KEY_9` |
| Brackets | `KEY_LEFTBRACE`, `KEY_RIGHTBRACE` (`[` and `]`) |
| Special | `KEY_ENTER`, `KEY_TAB`, `KEY_ESC`, `KEY_BACKSPACE`, `KEY_DELETE`, `KEY_SPACE`, `KEY_INSERT` |

## Auto-start on Boot

```bash
sudo systemctl enable --now mx3
sudo systemctl status mx3
sudo journalctl -u mx3 -f
```

To customize the service (for example, to use a different config file):

```bash
sudo systemctl edit mx3
# Add:
# [Service]
# ExecStart=
# ExecStart=/usr/bin/mx3 --config /path/to/config.conf
```

`mx3` does not read an `MX3_CONFIG` environment variable. Override `ExecStart`
if you need to pass runtime flags managed by `systemd`.

Or replace the full unit directly:

```bash
sudo systemctl edit --full mx3
```

## Finding Your Device Path

If auto-detection fails, find your mouse manually:

```bash
# List all input devices
cat /proc/bus/input/devices | grep -A5 -i logitech

# Look for the event handler (e.g., event5), then:
mx3 -d /dev/input/event5
```

## Permissions

The driver needs access to two device nodes:

| Path | Purpose | Production path |
|------|---------|-----------------|
| `/dev/input/event*` | Read mouse events | Use the packaged `systemd` service |
| `/dev/uinput` | Create virtual keyboard | Load `uinput` and use the packaged `systemd` service |

```bash
sudo modprobe uinput
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo systemctl daemon-reload
sudo systemctl enable --now mx3
```

For production deployments, `mx3` is expected to run as the packaged
root-managed `systemd` service. Manual local runs are still useful for
debugging, but they require direct access to the input devices on the host.

For that reason, a successful install does not imply `mx3` will run correctly
as an unprivileged interactive command. The supported new-user flow is:

1. `sudo make install`
2. `sudo modprobe uinput`
3. `sudo udevadm control --reload-rules && sudo udevadm trigger`
4. `sudo systemctl daemon-reload`
5. `sudo systemctl enable --now mx3`
6. `sudo systemctl status mx3`
7. `sudo journalctl -u mx3 -f`

The shipped `udev` rule does not grant broad interactive access to input
devices. That is intentional for production packaging.

## Debugging

```bash
# Verbose foreground logging
sudo mx3 -l debug

# Test with a specific device
sudo mx3 -l debug -d /dev/input/event5

# Check if uinput is available
ls -la /dev/uinput

# Check service logs
sudo journalctl -u mx3 -n 50 --no-pager

# Monitor generated key events
sudo evtest /dev/input/by-id/*MX3-Gesture-Driver*
```

## Build Options

```bash
make              # Release build (-O2)
make debug        # Debug build with AddressSanitizer
make test         # Run test suite
make verify-version
make release-artifacts
make lint         # Static analysis (needs cppcheck, clang-tidy)
make format       # Auto-format code (needs clang-format)
make install      # Install to /usr/local
make uninstall    # Remove system install and stop/disable mx3.service
make uninstall-local  # Remove ~/.local/bin/mx3 and ~/.config/mx3
```

Release and versioning are documented in `docs/release-process.md`.

## How It Works

1. Scans `/dev/input/event*` for a device matching the configured VID/PID or name
2. Opens `/dev/uinput` and registers as a virtual keyboard
3. Reads mouse events in a blocking loop
4. When a configured button (e.g., BTN_FORWARD) is pressed, starts accumulating relative X/Y motion
5. On button release, classifies the motion into a direction (tap, left, right, up, down, diagonal)
6. Looks up the action for that button+direction in the config
7. Sends the corresponding key combination through the virtual keyboard
8. Handles SIGINT/SIGTERM/SIGHUP for clean shutdown (removes the virtual device)

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "Cannot open /dev/uinput" | If running manually, use `sudo mx3 ...`; for normal installs, use `sudo systemctl enable --now mx3` and inspect `journalctl -u mx3` |
| No gestures detected | Check `-l debug` output, ensure device name matches |
| Wrong device selected | Use `-d /dev/input/eventN` to force a device |
| Keys not working | Verify key names in config are valid |
| "Permission denied" | Use the packaged `systemd` service, or run with device access for debugging |

## License

MIT — see [LICENSE](LICENSE)

## Credits

Based on the original [MX3-Linux-Driver](https://github.com/X4ndras/MX3-Linux-Driver) by Xandras.
