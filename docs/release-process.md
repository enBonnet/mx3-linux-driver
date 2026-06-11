# Release Process

## Version Source

`VERSION` is the single source of truth for the application release version.

The following files must stay aligned with it:

- `VERSION`
- `PKGBUILD`
- `mx3.spec`
- `debian/changelog`

Run:

```bash
make verify-version
```

before tagging a release.

## Release Checklist

1. Update `VERSION`.
2. Update `PKGBUILD`, `mx3.spec`, and `debian/changelog` to match.
3. Replace the Arch checksum placeholder in `PKGBUILD` after building the release tarball.
4. Run `make test`.
5. Run `make validate-packaging`.
6. Run `make verify-version`.
7. Run `make release-artifacts`.
8. Use the generated tarball checksum to update `PKGBUILD`.
9. Re-run `make verify-version` and `make validate-packaging`.
10. Build distro packages:
    - `dpkg-buildpackage -b -uc -us`
    - `rpmbuild -ba mx3.spec`
    - `makepkg`
    - `copr-cli build enbonnet/mx3 ~/rpmbuild/SRPMS/mx3-*.src.rpm`
11. Validate package install and service behavior on target distros.
12. Tag and publish the release.

## Cross-Distro Validation

Minimum validation set for each release:

- Debian or Ubuntu
- Fedora (via COPR)
- Arch Linux

For each distro family, verify:

- package build succeeds
- package install succeeds
- `mx3.service` starts
- reboot persistence works
- uninstall or remove behavior is clean
