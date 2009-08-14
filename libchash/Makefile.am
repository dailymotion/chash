#ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = bootstrap.sh
SUBDIRS=libchash

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libchash.pc

test: check
	libchash/chash_test

deb:
	debuild -i -us -uc -b
