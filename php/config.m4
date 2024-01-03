
PHP_ARG_ENABLE(chash, whether to enable consistent hash extension support,
[  --enable-chash          Enable consistent hash extension support])

if test "$PHP_CHASH" != "no"; then
  PKG_CHECK_MODULES(CHASH, [libchash >= 1.0])

  AC_DEFINE(HAVE_CHASH, 1, [Whether you have consistent hash extension support])

  PHP_EVAL_LIBLINE($CHASH_LIBS, CHASH_SHARED_LIBADD)

  PHP_NEW_EXTENSION(chash, chash.c, $ext_shared, $CHASH_CFLAGS)
  PHP_SUBST(CHASH_SHARED_LIBADD)
fi
