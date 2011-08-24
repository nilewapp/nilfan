# Maintainer: Allan McRae <allan@archlinux.org>

pkgname=nilfan
pkgver=1.0
pkgrel=1
pkgdesc="Automatically adjust the fan on a MacBook Pro"
arch=('i686' 'x86_64')
license=('GPL3')
source=(nilfan.c 
        nilfan.rc
        nilfan.conf)
md5sums=('59463ae04469f94e4fc7fc90beac322d'
         'c78ae49062cbabd9202f93f45ff06c0a'
         'b804e802052590b77f6447441fc4fc6e')
build() {
  cd $srcdir/
  gcc -o nilfan -lm -Wall $CFLAGS $LDFLAGS nilfan.c
}

package() {
  install -Dm755 $srcdir/nilfan $pkgdir/usr/bin/nilfan
  install -Dm755 $srcdir/nilfan.rc $pkgdir/etc/rc.d/nilfan
  install -Dm755 $srcdir/nilfan.conf $pkgdir/etc/nilfan.conf
}
