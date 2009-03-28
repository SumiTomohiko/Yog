#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(Yog, 0.9.0.0, no-define)
AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([cunit], [CU_initialize_registry], with_cunit="yes", with_cunit="no")

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h strings.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit bzero])

AC_CONFIG_FILES([Makefile src/Makefile tools/lemon/Makefile])
if test "${with_cunit}" = "yes"; then
    AC_CONFIG_FILES([tests/copying/Makefile 
                     tests/mark-sweep-compact/Makefile
                     tests/generational/Makefile])
fi
AC_CONFIG_SUBDIRS([bdwgc onig])
AC_OUTPUT

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
