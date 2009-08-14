PHP_ARG_ENABLE(chash, whether to enable consistent hash extension support,
[  --enable-chash          Enable consistent hash extension support])

if test "$PHP_CHASH" != "no"; then
  if ! test -f /usr/include/chash.h; then
    AC_MSG_ERROR([The libchash-dev package is not installed])
  fi

  AC_DEFINE(HAVE_CHASH, 1, [Whether you have consistent hash extension support])

  PHP_ADD_LIBRARY_WITH_PATH(chash, /usr/lib, CHASH_SHARED_LIBADD)
  PHP_ADD_INCLUDE(/usr/include)

  PHP_NEW_EXTENSION(chash, chash.c, $ext_shared)
  PHP_SUBST(CHASH_SHARED_LIBADD)
fi
