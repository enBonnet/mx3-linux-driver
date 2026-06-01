# Maintainer: Xandras <your-email@example.com>
# Contributor: Xandras <your-email@example.com>

pkgname=mx3
pkgver=1.0.0
pkgrel=1
pkgdesc="Gesture remapping driver for Logitech MX Master 3 mice on Linux"
arch=('x86_64')
url="https://github.com/Xandras/MX3-Linux-Driver"
license=('MIT')
depends=('glibc')
makedepends=('gcc')
optdepends=('systemd: for automatic startup at boot')
conflicts=('mx3-git')

source=("${pkgname}-${pkgver}.tar.gz::https://github.com/Xandras/MX3-Linux-Driver/archive/refs/tags/v${pkgver}.tar.gz")
sha256sums=('SKIP')

backup=('etc/mx3/config.conf')

package() {
    cd "${srcdir}/MX3-Linux-Driver-${pkgver}"

    make PREFIX=/usr DESTDIR="${pkgdir}" install
}
