Name:           mx3
Version:        1.0.1
Release:        2%{?dist}
Summary:        Gesture remapping driver for Logitech MX Master 3 mice

%global debug_package %{nil}

License:        MIT
URL:            https://github.com/enBonnet/mx3-linux-driver
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc make systemd-rpm-macros
Requires:       coreutils, glibc
Recommends:     systemd, udev

%description
MX3 captures mouse button holds and movement gestures from Logitech MX
Master 3 (and compatible) mice and translates them into keyboard
shortcuts via /dev/uinput.

Supports tap, swipe (4 cardinal), and diagonal (4 corner) gestures for
forward, back, middle, extra, side, and thumb gesture buttons.

Supported mice: MX Master 3, MX Master 3S, MX Master 2S, MX Anywhere 3,
and generic Logitech receivers (046d:*).

%prep
%setup -q -n MX3-Linux-Driver-%{version}

%build
make PREFIX=/usr

%install
make PREFIX=/usr UDEV_DIR=%{_prefix}/lib/udev/rules.d DESTDIR=%{buildroot} install

%post
%systemd_post mx3.service
udevadm control --reload-rules 2>/dev/null || true
udevadm trigger 2>/dev/null || true

%preun
%systemd_preun mx3.service

%postun
%systemd_postun_with_restart mx3.service

%files
%{_bindir}/mx3
%{_unitdir}/mx3.service
%config(noreplace) %{_sysconfdir}/mx3/config.conf
%{_udevrulesdir}/99-mx3.rules
%doc %{_docdir}/%{name}/README.md
%license %{_datadir}/licenses/%{name}/LICENSE

%changelog
* Tue Jun 02 2026 Ender Bonnet <enbonnet@gmail.com> - 1.0.1-1
- Fix uinput write return value handling
- Fix debug build linker flags for ASan/UBSan
- Fix Debian packaging for debhelper compat 13
- Fix test harness under dpkg-buildpackage

* Mon Jun 01 2026 Ender Bonnet <enbonnet@gmail.com> - 1.0.0-1
- Initial release
