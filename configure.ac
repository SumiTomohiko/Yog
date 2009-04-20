#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(Yog, 0.9.0.0, no-define)
AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([cunit], [CU_initialize_registry], with_cunit="yes", with_cunit="no")
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h limits.h stddef.h stdint.h stdlib.h string.h strings.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_UINT8_T
AC_CHECK_SIZEOF(void*)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gettimeofday isascii memchr memmove memset munmap strchr strrchr strstr strtol])

AC_ARG_ENABLE(profile, 
              AC_HELP_STRING([--enable-profile], 
                             [enable gprof profiling [[default=no]]]))
if test "${enable_profile}" = "yes"; then
    CFLAGS="-pg ${CFLAGS}"
    LDFLAGS="-pg ${LDFLAGS}"
fi
AC_ARG_ENABLE(google-perftools, 
              AC_HELP_STRING([--enable-google-perftools], 
                             [enable google-perftools [[default=no]]]))
if test "${enable_google_perftools}" = "yes"; then
    LDFLAGS="-lprofiler ${LDFLAGS}"
fi

AC_CONFIG_FILES([Makefile src/Makefile tools/lemon/Makefile])
if test "${with_cunit}" = "yes"; then
    AC_CONFIG_FILES([tests/copying/Makefile 
                     tests/mark-sweep-compact/Makefile
                     tests/generational/Makefile])
fi
AC_CONFIG_SUBDIRS([bdwgc
                   onig
                   sigsegv])
AC_OUTPUT

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
