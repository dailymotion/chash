AC_PREREQ(2.62)
AC_INIT([libchash],[1.0],[pyke (at) dailymotion (dot) com])
AC_CONFIG_SRCDIR([libchash/chash.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign 1.9 -Wall])

AC_SUBST(LIBCHASH_VERSION_INFO, [1:0:0])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

dnl Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h unistd.h])

dnl Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset munmap pow sqrt strdup])

AC_CONFIG_FILES([Makefile libchash/Makefile libchash.pc])
AC_OUTPUT
