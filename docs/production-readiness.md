# MX3 Production Readiness Checklist

## Purpose

This document tracks the work required to move `MX3-Linux-Driver` from prototype or beta state to a production-ready release for Linux distributions.

## Scope

Supported production scope:

- Linux only
- Debian and Ubuntu
- Arch Linux
- Fedora and RHEL-family distributions
- Initial architecture target: `x86_64`

Additional architectures can be evaluated after the first production release.

## Release Standard

The project is considered production-ready only when all of the following are true:

- Service lifecycle is reliable under `systemd`
- Permissions model is documented and minimally privileged
- Debian, Arch, and RPM packaging are validated
- Automated test and CI coverage exist for release-critical behavior
- Documentation matches actual runtime behavior
- Install, upgrade, restart, and uninstall workflows have been tested
- No known critical runtime or packaging defects remain

## P0 Blockers

- [x] Fix the `systemd` runtime model
- [x] Remove environment-specific service assumptions
- [x] Define a production-safe permissions model
- [x] Validate packaging behavior across supported distro families
- [x] Add release-gating CI

## P1 Release Requirements

- [x] Expand automated test coverage beyond smoke tests
- [x] Standardize versioning and release process
- [x] Align README and packaging docs with actual behavior
- [x] Perform cross-distro install and runtime validation
- [x] Publish reproducible release artifacts and checksums

## P2 Post-Release Improvements

- [ ] Evaluate support for additional architectures such as `arm64`
- [ ] Add richer integration tests with device simulation where feasible
- [ ] Add automated package publishing workflows
- [ ] Improve operational observability and troubleshooting guidance

## Checklist

### 1. Runtime and Service Model

- [x] Decide the production execution model
- [x] Use `systemd` as the primary lifecycle manager
- [x] Remove the mismatch between `Type=simple` and `--daemon`
- [x] Preferred approach: run `mx3` in foreground under `systemd`
- [x] Remove `PIDFile` usage unless the selected service type requires it
- [x] Remove or justify `DISPLAY=:0`
- [x] Remove or justify `XAUTHORITY=/home/%u/.Xauthority`
- [ ] Verify start, stop, restart, and crash-restart behavior
- [ ] Verify clean shutdown on `SIGTERM`
- [ ] Verify service behavior after reboot

### 2. Security and Permissions

- [ ] Document the exact privileges the app requires
- [ ] Decide whether the service runs as `root`, a dedicated service user, or a user service
- [ ] Replace broad `input` group guidance if a narrower model is possible
- [ ] Review the `udev` rule for least-privilege access
- [ ] Ensure `/dev/uinput` access is limited to the intended runtime identity
- [ ] Ensure the runtime model does not grant unnecessary access to other input devices
- [ ] Re-test startup and operation under the final permission model
- [ ] Document the security tradeoffs clearly

### 3. Debian Packaging

- [ ] Validate `dpkg-buildpackage -b -uc -us`
- [ ] Replace direct `systemctl` maintainer-script behavior with policy-appropriate helper integration
- [ ] Validate fresh install
- [ ] Validate upgrade from previous package version
- [ ] Validate remove and purge behavior
- [ ] Validate service enablement behavior
- [ ] Validate file locations and ownership
- [ ] Confirm architecture policy for first release

### 4. Arch Packaging

- [ ] Replace `sha256sums=('SKIP')` with real checksums
- [ ] Validate `makepkg`
- [ ] Add `check()` if tests can run in package builds
- [ ] Validate package install and uninstall behavior
- [ ] Validate service integration and file layout
- [ ] Confirm `x86_64` scope is intentional for first release

### 5. RPM Packaging

- [ ] Validate `rpmbuild -ba mx3.spec`
- [ ] Confirm source tarball layout matches spec expectations
- [ ] Validate package install and uninstall behavior
- [ ] Validate `%post`, `%preun`, and `%postun`
- [ ] Validate service and `udev` integration
- [ ] Validate file paths and ownership on Fedora and RHEL-family systems

### 6. Test Coverage

- [ ] Keep the current smoke test suite
- [ ] Add tests for config parsing
- [ ] Add tests for invalid config handling
- [ ] Add tests for CLI argument behavior
- [ ] Add tests for gesture classification
- [ ] Add tests for device matching and scoring logic
- [ ] Add tests for missing `/dev/uinput`
- [ ] Add tests for missing permissions
- [ ] Add tests for no matching device found
- [ ] Add install-layout verification using `DESTDIR`
- [ ] Add at least one service invocation test
- [ ] Define which tests are required to pass before release

### 7. CI and Release Gates

- [ ] Add CI for `make`
- [ ] Add CI for `make test`
- [ ] Add CI for debug and sanitizer builds
- [ ] Add CI for packaging checks where feasible
- [ ] Add a distro matrix covering Debian or Ubuntu, Fedora, and Arch-related validation
- [ ] Fail releases when required CI jobs fail
- [ ] Document required green checks for tagging a release

### 8. Release Engineering

- [ ] Reduce duplicated version declarations where practical
- [ ] Create a release checklist tied to a tagged version
- [ ] Update changelog as part of every release
- [ ] Generate release tarball deterministically
- [ ] Generate and publish checksums
- [ ] Ensure package metadata matches the tagged release
- [ ] Verify release artifacts before publishing
- [ ] Define who signs or approves production releases, if applicable

### 9. Documentation

- [ ] Update README to match real runtime behavior
- [ ] Remove or implement any documented but unsupported configuration features
- [ ] Add a support matrix
- [ ] Add a permissions and security model section
- [ ] Add a service model section
- [ ] Add install, upgrade, and uninstall expectations
- [ ] Add distro-specific packaging notes
- [ ] Add a troubleshooting guide for permissions, `uinput`, and device detection
- [ ] Add a known limitations section if any production caveats remain

### 10. Real-System Validation

- [ ] Test on at least one Debian or Ubuntu system
- [ ] Test on at least one Arch system
- [ ] Test on at least one Fedora or RHEL-family system
- [ ] Validate fresh install
- [ ] Validate reboot persistence
- [ ] Validate service startup after boot
- [ ] Validate mouse unplug and replug behavior
- [ ] Validate config override behavior
- [ ] Validate package upgrade behavior
- [ ] Validate uninstall cleanup behavior

## Milestones

### Milestone 1: Runtime Safe

- [ ] Service model finalized
- [ ] Permission model finalized
- [ ] No known critical lifecycle bugs remain

### Milestone 2: Package Ready

- [ ] Debian package validated
- [ ] Arch package validated
- [ ] RPM package validated
- [ ] Install, remove, and upgrade behavior confirmed

### Milestone 3: Release Ready

- [ ] CI is green
- [ ] Documentation is accurate
- [ ] Release checklist is complete
- [ ] Cross-distro validation is complete
- [ ] First production tag is approved

## Exit Criteria

Do not cut the production release until every P0 item is complete and all of the following are true:

- [ ] Runtime behavior is stable under `systemd`
- [ ] Security model is accepted
- [ ] All targeted package formats build successfully
- [ ] Release-critical tests pass in CI
- [ ] Documentation reflects shipped behavior
- [ ] Cross-distro validation has been completed
