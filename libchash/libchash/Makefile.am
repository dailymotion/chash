lib_LTLIBRARIES=libchash.la
libchash_la_SOURCES=chash.c chash.h
libchash_la_LDFLAGS=-version-info $(LIBCHASH_VERSION_INFO)

check_PROGRAMS=chash_test
chash_test_SOURCES=chash_test.c
chash_test_LDADD=libchash.la -lm

include_HEADERS=chash.h