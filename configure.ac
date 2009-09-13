#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(Yog, 0.0.2, SumiTomohiko@neko-daisuki.ddo.jp)
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
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([dl], [dlopen])

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
AC_CHECK_FUNCS([atexit gettimeofday isascii memchr memmove memset munmap strchr strrchr strstr strtol dlopen])
AC_CHECK_HEADERS([dlfcn.h sys/mman.h windows.h])

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

AC_CONFIG_FILES([Makefile src/Makefile tools/lemon/Makefile tests/Makefile
                 ext/Makefile])

AC_CANONICAL_HOST
case "${host}" in
*-openbsd*)
    TEST_TARGETS="${TEST_TARGETS} test-copying test-mark-sweep test-mark-sweep-compact test-generational"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-copying\$(EXEEXT) yog-mark-sweep\$(EXEEXT) yog-mark-sweep-compact\$(EXEEXT) yog-generational\$(EXEEXT) miniyog\$(EXEEXT)"
    EXTS="concurrent\$(SOEXT)"
    SOEXT=".so"
    ;;
*-mingw32)
    PTHREADS_W32_DIR="`pwd`/pthreads-w32"
    CFLAGS="${CFLAGS} -I${PTHREADS_W32_DIR}"
    LDFLAGS_COMMON="${LDFLAGS_COMMON} ${PTHREADS_W32_DIR}/libpthreadGC2.a -lwsock32"
    TEST_TARGETS="${TEST_TARGETS} test-copying test-mark-sweep"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-copying\$(EXEEXT) yog-mark-sweep\$(EXEEXT) miniyog\$(EXEEXT)"
    SOEXT=".dll"
    ;;
*)
    AC_CONFIG_SUBDIRS([bdwgc])
    DIRS="${DIRS} bdwgc"
    TEST_TARGETS="${TEST_TARGETS} test-bdw test-copying test-mark-sweep test-mark-sweep-compact test-generational"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-bdw\$(EXEEXT) yog-copying\$(EXEEXT) yog-mark-sweep\$(EXEEXT) yog-mark-sweep-compact\$(EXEEXT) yog-generational\$(EXEEXT) miniyog\$(EXEEXT)"
    EXTS="concurrent\$(SOEXT)"
    SOEXT=".so"
    ;;
esac
DIRS="${DIRS} onig gmp src ext"

AC_SUBST(DIRS)
AC_SUBST(PTHREADS_W32_DIR)
AC_SUBST(TEST_TARGETS)
AC_SUBST(BIN_PROGRAMS)
AC_SUBST(PACKAGE_VERSION)
AC_SUBST(LDFLAGS_COMMON)
AC_SUBST(EXTS)
AC_SUBST(SOEXT)

AC_CONFIG_SUBDIRS([onig
                   gmp])
AC_OUTPUT

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
