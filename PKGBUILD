# Maintainer: Ender Bonnet <enbonnet@gmail.com>

pkgname=mx3
pkgver=1.0.0
pkgrel=1
pkgdesc="Gesture remapping driver for Logitech MX Master 3 mice on Linux"
arch=('x86_64')
url="https://github.com/enBonnet/mx3-linux-driver"
license=('MIT')
depends=('glibc')
makedepends=('gcc')
optdepends=('systemd: for automatic startup at boot')
conflicts=('mx3-git')

source=("${pkgname}-${pkgver}.tar.gz::https://github.com/enBonnet/mx3-linux-driver/archive/refs/tags/v${pkgver}.tar.gz")
sha256sums=('TO_BE_FILLED_IN_FOR_RELEASE')

backup=('etc/mx3/config.conf')

check() {
    cd "${srcdir}/MX3-Linux-Driver-${pkgver}"

    make test
    make validate-packaging
}

package() {
    cd "${srcdir}/MX3-Linux-Driver-${pkgver}"

    make PREFIX=/usr DESTDIR="${pkgdir}" install
}
