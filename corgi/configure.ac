#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([corgi], [0.0.2], [SumiTomohiko@neko-daisuki.ddo.jp])
AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_SRCDIR([src/corgi.c])
AC_CONFIG_HEADERS([include/corgi/config.h])
AX_PREFIX_CONFIG_H([include/corgi/config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
CORGI_PATH_PYTHON([3.0])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([alloca.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(void*)

# Checks for library functions.

AC_ARG_ENABLE(profile,
              AC_HELP_STRING([--enable-profile],
                             [enable gprof profiling [[default=no]]]))
if test "${enable_profile}" = "yes"; then
    CFLAGS="-pg ${CFLAGS}"
    LDFLAGS="-pg ${LDFLAGS}"
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile
                 rc
                 doc/conf.py])
AC_OUTPUT

# vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
