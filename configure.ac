#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(Yog, 0.0.5, SumiTomohiko@neko-daisuki.ddo.jp)
AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_SRCDIR([include/yog/config.h.in])
AC_CONFIG_HEADER([include/yog/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_CC_C_O

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([dl], [dlopen])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h limits.h stddef.h stdint.h stdlib.h string.h \
                  strings.h sys/time.h unistd.h malloc.h getopt.h float.h \
                  dlfcn.h sys/mman.h windows.h alloca.h winerror.h arpa/inet.h \
                  netdb.h sys/socket.h winsock2.h])
AC_CHECK_HEADERS([CUnit/CUnit.h], cunit=yes, cunit=no)

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
YOG_CHECK_ERRNO([E2BIG])
YOG_CHECK_ERRNO([EACCESS])
YOG_CHECK_ERRNO([EADDRINUSE])
YOG_CHECK_ERRNO([EADDRNOTAVAIL])
YOG_CHECK_ERRNO([EAFNOSUPPORT])
YOG_CHECK_ERRNO([EAGAIN])
YOG_CHECK_ERRNO([EALREADY])
YOG_CHECK_ERRNO([EBADE])
YOG_CHECK_ERRNO([EBADFD])
YOG_CHECK_ERRNO([EBADF])
YOG_CHECK_ERRNO([EBADMSG])
YOG_CHECK_ERRNO([EBADRQC])
YOG_CHECK_ERRNO([EBADR])
YOG_CHECK_ERRNO([EBADSLT])
YOG_CHECK_ERRNO([EBUSY])
YOG_CHECK_ERRNO([ECANCELED])
YOG_CHECK_ERRNO([ECHILD])
YOG_CHECK_ERRNO([ECHRNG])
YOG_CHECK_ERRNO([ECOMM])
YOG_CHECK_ERRNO([ECONNABORTED])
YOG_CHECK_ERRNO([ECONNREFUSED])
YOG_CHECK_ERRNO([ECONNRESET])
YOG_CHECK_ERRNO([EDEADLK])
YOG_CHECK_ERRNO([EDEADLOCK])
YOG_CHECK_ERRNO([EDESTADDRREQ])
YOG_CHECK_ERRNO([EDOM])
YOG_CHECK_ERRNO([EDQUOT])
YOG_CHECK_ERRNO([EEXIST])
YOG_CHECK_ERRNO([EFAULT])
YOG_CHECK_ERRNO([EFBIG])
YOG_CHECK_ERRNO([EHOSTDOWN])
YOG_CHECK_ERRNO([EHOSTUNREACH])
YOG_CHECK_ERRNO([EIDRM])
YOG_CHECK_ERRNO([EILSEQ])
YOG_CHECK_ERRNO([EINPROGRESS])
YOG_CHECK_ERRNO([EINTR])
YOG_CHECK_ERRNO([EINVAL])
YOG_CHECK_ERRNO([EIO])
YOG_CHECK_ERRNO([EISCONN])
YOG_CHECK_ERRNO([EISDIR])
YOG_CHECK_ERRNO([EISNAM])
YOG_CHECK_ERRNO([EKEYEXPIRED])
YOG_CHECK_ERRNO([EKEYREJECTED])
YOG_CHECK_ERRNO([EKEYREVOKED])
YOG_CHECK_ERRNO([EL2HLT])
YOG_CHECK_ERRNO([EL2NSYNC])
YOG_CHECK_ERRNO([EL3HLT])
YOG_CHECK_ERRNO([EL3RST])
YOG_CHECK_ERRNO([ELIBACC])
YOG_CHECK_ERRNO([ELIBBAD])
YOG_CHECK_ERRNO([ELIBEXEC])
YOG_CHECK_ERRNO([ELIBMAX])
YOG_CHECK_ERRNO([ELIBSCN])
YOG_CHECK_ERRNO([ELOOP])
YOG_CHECK_ERRNO([EMEDIUMTYPE])
YOG_CHECK_ERRNO([EMFILE])
YOG_CHECK_ERRNO([EMLINK])
YOG_CHECK_ERRNO([EMSGSIZE])
YOG_CHECK_ERRNO([EMULTIHOP])
YOG_CHECK_ERRNO([ENAMETOOLONG])
YOG_CHECK_ERRNO([ENETDOWN])
YOG_CHECK_ERRNO([ENETRESET])
YOG_CHECK_ERRNO([ENETUNREACH])
YOG_CHECK_ERRNO([ENFILE])
YOG_CHECK_ERRNO([ENOBUFS])
YOG_CHECK_ERRNO([ENODATA])
YOG_CHECK_ERRNO([ENODEV])
YOG_CHECK_ERRNO([ENOENT])
YOG_CHECK_ERRNO([ENOEXEC])
YOG_CHECK_ERRNO([ENOKEY])
YOG_CHECK_ERRNO([ENOLCK])
YOG_CHECK_ERRNO([ENOLINK])
YOG_CHECK_ERRNO([ENOMEDIUM])
YOG_CHECK_ERRNO([ENOMEM])
YOG_CHECK_ERRNO([ENOMSG])
YOG_CHECK_ERRNO([ENONET])
YOG_CHECK_ERRNO([ENOPKG])
YOG_CHECK_ERRNO([ENOPROTOOPT])
YOG_CHECK_ERRNO([ENOSPC])
YOG_CHECK_ERRNO([ENOSR])
YOG_CHECK_ERRNO([ENOSTR])
YOG_CHECK_ERRNO([ENOSYS])
YOG_CHECK_ERRNO([ENOTBLK])
YOG_CHECK_ERRNO([ENOTCONN])
YOG_CHECK_ERRNO([ENOTDIR])
YOG_CHECK_ERRNO([ENOTEMPTY])
YOG_CHECK_ERRNO([ENOTSOCK])
YOG_CHECK_ERRNO([ENOTSUP])
YOG_CHECK_ERRNO([ENOTTY])
YOG_CHECK_ERRNO([ENOTUNIQ])
YOG_CHECK_ERRNO([ENXIO])
YOG_CHECK_ERRNO([EOPNOTSUPP])
YOG_CHECK_ERRNO([EOVERFLOW])
YOG_CHECK_ERRNO([EPERM])
YOG_CHECK_ERRNO([EPFNOSUPPORT])
YOG_CHECK_ERRNO([EPIPE])
YOG_CHECK_ERRNO([EPROTONOSUPPORT])
YOG_CHECK_ERRNO([EPROTOTYPE])
YOG_CHECK_ERRNO([EPROTO])
YOG_CHECK_ERRNO([ERANGE])
YOG_CHECK_ERRNO([EREMCHG])
YOG_CHECK_ERRNO([EREMOTEIO])
YOG_CHECK_ERRNO([EREMOTE])
YOG_CHECK_ERRNO([ERESTART])
YOG_CHECK_ERRNO([EROFS])
YOG_CHECK_ERRNO([ESHUTDOWN])
YOG_CHECK_ERRNO([ESOCKTNOSUPPORT])
YOG_CHECK_ERRNO([ESPIPE])
YOG_CHECK_ERRNO([ESRCH])
YOG_CHECK_ERRNO([ESTALE])
YOG_CHECK_ERRNO([ESTRPIPE])
YOG_CHECK_ERRNO([ETIMEDOUT])
YOG_CHECK_ERRNO([ETIME])
YOG_CHECK_ERRNO([ETXTBSY])
YOG_CHECK_ERRNO([EUCLEAN])
YOG_CHECK_ERRNO([EUNATCH])
YOG_CHECK_ERRNO([EUSERS])
YOG_CHECK_ERRNO([EWOULDBLOCK])
YOG_CHECK_ERRNO([EXDEV])
YOG_CHECK_ERRNO([EXFULL])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gettimeofday isascii memchr memmove memset munmap \
                strchr strrchr strstr strtol dlopen isnan alloca vsnprintf \
                pthread_rwlockattr_setkind_np pthread_rwlockattr_init \
                pthread_rwlockattr_destroy snprintf _isnan mkdir getaddrinfo \
                bzero])

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

AM_PATH_PYTHON()

AC_DEFINE_UNQUOTED([PREFIX], ["$prefix"], [Define install directory])

AC_CONFIG_FILES([Makefile src/Makefile tools/lemon/Makefile tests/Makefile
                 ext/Makefile lib/Makefile doc/Makefile installer.nsi
                 ext/concurrent/Makefile ext/yaml/Makefile ext/zlib/Makefile
                 ext/zip/Makefile ext/socket/Makefile ext/Makefile.common])
if test "${cunit}" == "yes"
then
    AC_CONFIG_FILES([tests/gc/Makefile])
fi

CFLAGS="${CFLAGS} -Wall -I`pwd`/include -I`pwd`/onig"

AC_CANONICAL_HOST
case "${host}" in
*-openbsd*)
    CFLAGS="${CFLAGS} -pthread"
    LDFLAGS_COMMON="${LDFLAGS_COMMON} -ggdb -export-dynamic"
    TEST_TARGETS="${TEST_TARGETS} test-copying test-mark-sweep test-mark-sweep-compact test-generational"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-copying\$(EXEEXT) yog-mark-sweep\$(EXEEXT) yog-mark-sweep-compact\$(EXEEXT) yog-generational\$(EXEEXT) miniyog\$(EXEEXT)"
    SOEXT="so"
    EXT_SUBDIRS="concurrent"
    ;;
*-mingw32)
    CFLAGS="${CFLAGS} -I/mingw32/include"
    LDFLAGS_COPYING="${LDFLAGS_COPYING} -Wl,--out-implib,yog-copying.lib"
    LDFLAGS_MINIYOG="${LDFLAGS_MINIYOG} -Wl,--out-implib,miniyog.lib"
    LDFLAGS_COMMON="${LDFLAGS_COMMON} --enable-runtime-pseudo-reloc"
    LIBS="${LIBS} /lib/libpthread.dll.a -lws2_32"
    TEST_TARGETS="${TEST_TARGETS} test-copying"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-copying\$(EXEEXT) miniyog\$(EXEEXT)"
    SOEXT="dll"
    IMPORT_LIB="\$(top_srcdir)/src/yog-copying.lib"
    ;;
*)
    case "${host}" in
    *-linux*)
        CFLAGS="${CFLAGS} -D_XOPEN_SOURCE=500 -D_BSD_SOURCE"
        ;;
    esac
    LDFLAGS_COMMON="${LDFLAGS_COMMON} -export-dynamic"
    AC_CONFIG_SUBDIRS([bdwgc])
    DIRS="${DIRS} bdwgc"
    TEST_TARGETS="${TEST_TARGETS} test-bdw test-copying test-mark-sweep test-mark-sweep-compact test-generational"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-bdw\$(EXEEXT) yog-copying\$(EXEEXT) yog-mark-sweep\$(EXEEXT) yog-mark-sweep-compact\$(EXEEXT) yog-generational\$(EXEEXT) miniyog\$(EXEEXT)"
    SOEXT="so"
    EXT_SUBDIRS="concurrent"
    ;;
esac
DIRS="${DIRS} onig gmp src ext"

YOG_CHECK_EXT([yaml], [syck_new_parser], [syck])
YOG_CHECK_EXT([z], [zlibVersion], [zlib])
YOG_CHECK_EXT([zip], [zip_open], [zip])

AC_SUBST(DIRS)
AC_SUBST(TEST_TARGETS)
AC_SUBST(BIN_PROGRAMS)
AC_SUBST(PACKAGE_VERSION)
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS_COMMON)
AC_SUBST(LDFLAGS_BDW)
AC_SUBST(LDFLAGS_COPYING)
AC_SUBST(LDFLAGS_GENERATIONAL)
AC_SUBST(LDFLAGS_MARK_SWEEP)
AC_SUBST(LDFLAGS_MARK_SWEEP_COMPACT)
AC_SUBST(LDFLAGS_MINIYOG)
AC_SUBST(LIBS)
AC_SUBST(SOEXT)
AC_SUBST(IMPORT_LIB)
AC_SUBST(EXT_SUBDIRS)

AC_CONFIG_SUBDIRS([onig gmp tools/swig])

AC_OUTPUT

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
