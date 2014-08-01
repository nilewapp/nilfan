pkgname=simplefan
pkgver=1.0.1
pkgrel=1
pkgdesc="Fan Control daemon for MacBook Pro laptops"
arch=('i686' 'x86_64')
license=('GPL3')
source=(simplefan.c
        simplefan.service
        simplefan.conf)
build() {
  cd $srcdir/
  gcc -o simplefan -lm -Wall simplefan.c
}

package() {
  install -Dm755 $srcdir/simplefan $pkgdir/usr/bin/simplefan
  install -Dm755 $srcdir/simplefan.service $pkgdir/etc/systemd/system/simplefan.service
  install -Dm644 $srcdir/simplefan.conf $pkgdir/etc/simplefan.conf
}
md5sums=('3c275275bb9af991e0b691db6b3a184d'
         '358f8d3c4e4afcb4d5d7810804ae491d'
         'b804e802052590b77f6447441fc4fc6e')
