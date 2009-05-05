PHP_ARG_ENABLE(chash, whether to enable consistent hash extension support,
[  --enable-chash          Enable consistent hash extension support])

if test "$PHP_CHASH" = "yes"; then
  AC_DEFINE(HAVE_CHASH, 1, [Whether you have consistent hash extension support])
  PHP_NEW_EXTENSION(chash, chash.c, $ext_shared)
fi
