#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(Yog, 0.0.4, SumiTomohiko@neko-daisuki.ddo.jp)
AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_SRCDIR([include/yog/config.h.in])
AC_CONFIG_HEADER([include/yog/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_CC_C_O
#AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([dl], [dlopen])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h limits.h stddef.h stdint.h stdlib.h string.h \
                  strings.h sys/time.h unistd.h malloc.h getopt.h float.h \
                  dlfcn.h sys/mman.h windows.h alloca.h])

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
AC_CHECK_DECL(E2BIG, [AC_DEFINE([HAVE_E2BIG], [1], [E2BIG])], , [#include <errno.h>])
AC_CHECK_DECL(EACCES, [AC_DEFINE([HAVE_EACCES], [1], [EACCES])], , [#include <errno.h>])
AC_CHECK_DECL(EADDRINUSE, [AC_DEFINE([HAVE_EADDRINUSE], [1], [EADDRINUSE])], , [#include <errno.h>])
AC_CHECK_DECL(EADDRNOTAVAIL, [AC_DEFINE([HAVE_EADDRNOTAVAIL], [1], [EADDRNOTAVAIL])], , [#include <errno.h>])
AC_CHECK_DECL(EAFNOSUPPORT, [AC_DEFINE([HAVE_EAFNOSUPPORT], [1], [EAFNOSUPPORT])], , [#include <errno.h>])
AC_CHECK_DECL(EAGAIN, [AC_DEFINE([HAVE_EAGAIN], [1], [EAGAIN])], , [#include <errno.h>])
AC_CHECK_DECL(EALREADY, [AC_DEFINE([HAVE_EALREADY], [1], [EALREADY])], , [#include <errno.h>])
AC_CHECK_DECL(EBADE, [AC_DEFINE([HAVE_EBADE], [1], [EBADE])], , [#include <errno.h>])
AC_CHECK_DECL(EBADF, [AC_DEFINE([HAVE_EBADF], [1], [EBADF])], , [#include <errno.h>])
AC_CHECK_DECL(EBADFD, [AC_DEFINE([HAVE_EBADFD], [1], [EBADFD])], , [#include <errno.h>])
AC_CHECK_DECL(EBADMSG, [AC_DEFINE([HAVE_EBADMSG], [1], [EBADMSG])], , [#include <errno.h>])
AC_CHECK_DECL(EBADR, [AC_DEFINE([HAVE_EBADR], [1], [EBADR])], , [#include <errno.h>])
AC_CHECK_DECL(EBADRQC, [AC_DEFINE([HAVE_EBADRQC], [1], [EBADRQC])], , [#include <errno.h>])
AC_CHECK_DECL(EBADSLT, [AC_DEFINE([HAVE_EBADSLT], [1], [EBADSLT])], , [#include <errno.h>])
AC_CHECK_DECL(EBUSY, [AC_DEFINE([HAVE_EBUSY], [1], [EBUSY])], , [#include <errno.h>])
AC_CHECK_DECL(ECANCELED, [AC_DEFINE([HAVE_ECANCELED], [1], [ECANCELED])], , [#include <errno.h>])
AC_CHECK_DECL(ECHILD, [AC_DEFINE([HAVE_ECHILD], [1], [ECHILD])], , [#include <errno.h>])
AC_CHECK_DECL(ECHRNG, [AC_DEFINE([HAVE_ECHRNG], [1], [ECHRNG])], , [#include <errno.h>])
AC_CHECK_DECL(ECOMM, [AC_DEFINE([HAVE_ECOMM], [1], [ECOMM])], , [#include <errno.h>])
AC_CHECK_DECL(ECONNABORTED, [AC_DEFINE([HAVE_ECONNABORTED], [1], [ECONNABORTED])], , [#include <errno.h>])
AC_CHECK_DECL(ECONNREFUSED, [AC_DEFINE([HAVE_ECONNREFUSED], [1], [ECONNREFUSED])], , [#include <errno.h>])
AC_CHECK_DECL(ECONNRESET, [AC_DEFINE([HAVE_ECONNRESET], [1], [ECONNRESET])], , [#include <errno.h>])
AC_CHECK_DECL(EDEADLK, [AC_DEFINE([HAVE_EDEADLK], [1], [EDEADLK])], , [#include <errno.h>])
AC_CHECK_DECL(EDEADLOCK, [AC_DEFINE([HAVE_EDEADLOCK], [1], [EDEADLOCK])], , [#include <errno.h>])
AC_CHECK_DECL(EDESTADDRREQ, [AC_DEFINE([HAVE_EDESTADDRREQ], [1], [EDESTADDRREQ])], , [#include <errno.h>])
AC_CHECK_DECL(EDOM, [AC_DEFINE([HAVE_EDOM], [1], [EDOM])], , [#include <errno.h>])
AC_CHECK_DECL(EDQUOT, [AC_DEFINE([HAVE_EDQUOT], [1], [EDQUOT])], , [#include <errno.h>])
AC_CHECK_DECL(EEXIST, [AC_DEFINE([HAVE_EEXIST], [1], [EEXIST])], , [#include <errno.h>])
AC_CHECK_DECL(EFAULT, [AC_DEFINE([HAVE_EFAULT], [1], [EFAULT])], , [#include <errno.h>])
AC_CHECK_DECL(EFBIG, [AC_DEFINE([HAVE_EFBIG], [1], [EFBIG])], , [#include <errno.h>])
AC_CHECK_DECL(EHOSTDOWN, [AC_DEFINE([HAVE_EHOSTDOWN], [1], [EHOSTDOWN])], , [#include <errno.h>])
AC_CHECK_DECL(EHOSTUNREACH, [AC_DEFINE([HAVE_EHOSTUNREACH], [1], [EHOSTUNREACH])], , [#include <errno.h>])
AC_CHECK_DECL(EIDRM, [AC_DEFINE([HAVE_EIDRM], [1], [EIDRM])], , [#include <errno.h>])
AC_CHECK_DECL(EILSEQ, [AC_DEFINE([HAVE_EILSEQ], [1], [EILSEQ])], , [#include <errno.h>])
AC_CHECK_DECL(EINPROGRESS, [AC_DEFINE([HAVE_EINPROGRESS], [1], [EINPROGRESS])], , [#include <errno.h>])
AC_CHECK_DECL(EINTR, [AC_DEFINE([HAVE_EINTR], [1], [EINTR])], , [#include <errno.h>])
AC_CHECK_DECL(EINVAL, [AC_DEFINE([HAVE_EINVAL], [1], [EINVAL])], , [#include <errno.h>])
AC_CHECK_DECL(EIO, [AC_DEFINE([HAVE_EIO], [1], [EIO])], , [#include <errno.h>])
AC_CHECK_DECL(EISCONN, [AC_DEFINE([HAVE_EISCONN], [1], [EISCONN])], , [#include <errno.h>])
AC_CHECK_DECL(EISDIR, [AC_DEFINE([HAVE_EISDIR], [1], [EISDIR])], , [#include <errno.h>])
AC_CHECK_DECL(EISNAM, [AC_DEFINE([HAVE_EISNAM], [1], [EISNAM])], , [#include <errno.h>])
AC_CHECK_DECL(EKEYEXPIRED, [AC_DEFINE([HAVE_EKEYEXPIRED], [1], [EKEYEXPIRED])], , [#include <errno.h>])
AC_CHECK_DECL(EKEYREJECTED, [AC_DEFINE([HAVE_EKEYREJECTED], [1], [EKEYREJECTED])], , [#include <errno.h>])
AC_CHECK_DECL(EKEYREVOKED, [AC_DEFINE([HAVE_EKEYREVOKED], [1], [EKEYREVOKED])], , [#include <errno.h>])
AC_CHECK_DECL(EL2HLT, [AC_DEFINE([HAVE_EL2HLT], [1], [EL2HLT])], , [#include <errno.h>])
AC_CHECK_DECL(EL2NSYNC, [AC_DEFINE([HAVE_EL2NSYNC], [1], [EL2NSYNC])], , [#include <errno.h>])
AC_CHECK_DECL(EL3HLT, [AC_DEFINE([HAVE_EL3HLT], [1], [EL3HLT])], , [#include <errno.h>])
AC_CHECK_DECL(EL3RST, [AC_DEFINE([HAVE_EL3RST], [1], [EL3RST])], , [#include <errno.h>])
AC_CHECK_DECL(ELIBACC, [AC_DEFINE([HAVE_ELIBACC], [1], [ELIBACC])], , [#include <errno.h>])
AC_CHECK_DECL(ELIBBAD, [AC_DEFINE([HAVE_ELIBBAD], [1], [ELIBBAD])], , [#include <errno.h>])
AC_CHECK_DECL(ELIBMAX, [AC_DEFINE([HAVE_ELIBMAX], [1], [ELIBMAX])], , [#include <errno.h>])
AC_CHECK_DECL(ELIBSCN, [AC_DEFINE([HAVE_ELIBSCN], [1], [ELIBSCN])], , [#include <errno.h>])
AC_CHECK_DECL(ELIBEXEC, [AC_DEFINE([HAVE_ELIBEXEC], [1], [ELIBEXEC])], , [#include <errno.h>])
AC_CHECK_DECL(ELOOP, [AC_DEFINE([HAVE_ELOOP], [1], [ELOOP])], , [#include <errno.h>])
AC_CHECK_DECL(EMEDIUMTYPE, [AC_DEFINE([HAVE_EMEDIUMTYPE], [1], [EMEDIUMTYPE])], , [#include <errno.h>])
AC_CHECK_DECL(EMFILE, [AC_DEFINE([HAVE_EMFILE], [1], [EMFILE])], , [#include <errno.h>])
AC_CHECK_DECL(EMLINK, [AC_DEFINE([HAVE_EMLINK], [1], [EMLINK])], , [#include <errno.h>])
AC_CHECK_DECL(EMSGSIZE, [AC_DEFINE([HAVE_EMSGSIZE], [1], [EMSGSIZE])], , [#include <errno.h>])
AC_CHECK_DECL(EMULTIHOP, [AC_DEFINE([HAVE_EMULTIHOP], [1], [EMULTIHOP])], , [#include <errno.h>])
AC_CHECK_DECL(ENAMETOOLONG, [AC_DEFINE([HAVE_ENAMETOOLONG], [1], [ENAMETOOLONG])], , [#include <errno.h>])
AC_CHECK_DECL(ENETDOWN, [AC_DEFINE([HAVE_ENETDOWN], [1], [ENETDOWN])], , [#include <errno.h>])
AC_CHECK_DECL(ENETRESET, [AC_DEFINE([HAVE_ENETRESET], [1], [ENETRESET])], , [#include <errno.h>])
AC_CHECK_DECL(ENETUNREACH, [AC_DEFINE([HAVE_ENETUNREACH], [1], [ENETUNREACH])], , [#include <errno.h>])
AC_CHECK_DECL(ENFILE, [AC_DEFINE([HAVE_ENFILE], [1], [ENFILE])], , [#include <errno.h>])
AC_CHECK_DECL(ENOBUFS, [AC_DEFINE([HAVE_ENOBUFS], [1], [ENOBUFS])], , [#include <errno.h>])
AC_CHECK_DECL(ENODATA, [AC_DEFINE([HAVE_ENODATA], [1], [ENODATA])], , [#include <errno.h>])
AC_CHECK_DECL(ENODEV, [AC_DEFINE([HAVE_ENODEV], [1], [ENODEV])], , [#include <errno.h>])
AC_CHECK_DECL(ENOENT, [AC_DEFINE([HAVE_ENOENT], [1], [ENOENT])], , [#include <errno.h>])
AC_CHECK_DECL(ENOEXEC, [AC_DEFINE([HAVE_ENOEXEC], [1], [ENOEXEC])], , [#include <errno.h>])
AC_CHECK_DECL(ENOKEY, [AC_DEFINE([HAVE_ENOKEY], [1], [ENOKEY])], , [#include <errno.h>])
AC_CHECK_DECL(ENOLCK, [AC_DEFINE([HAVE_ENOLCK], [1], [ENOLCK])], , [#include <errno.h>])
AC_CHECK_DECL(ENOLINK, [AC_DEFINE([HAVE_ENOLINK], [1], [ENOLINK])], , [#include <errno.h>])
AC_CHECK_DECL(ENOMEDIUM, [AC_DEFINE([HAVE_ENOMEDIUM], [1], [ENOMEDIUM])], , [#include <errno.h>])
AC_CHECK_DECL(ENOMEM, [AC_DEFINE([HAVE_ENOMEM], [1], [ENOMEM])], , [#include <errno.h>])
AC_CHECK_DECL(ENOMSG, [AC_DEFINE([HAVE_ENOMSG], [1], [ENOMSG])], , [#include <errno.h>])
AC_CHECK_DECL(ENONET, [AC_DEFINE([HAVE_ENONET], [1], [ENONET])], , [#include <errno.h>])
AC_CHECK_DECL(ENOPKG, [AC_DEFINE([HAVE_ENOPKG], [1], [ENOPKG])], , [#include <errno.h>])
AC_CHECK_DECL(ENOPROTOOPT, [AC_DEFINE([HAVE_ENOPROTOOPT], [1], [ENOPROTOOPT])], , [#include <errno.h>])
AC_CHECK_DECL(ENOSPC, [AC_DEFINE([HAVE_ENOSPC], [1], [ENOSPC])], , [#include <errno.h>])
AC_CHECK_DECL(ENOSR, [AC_DEFINE([HAVE_ENOSR], [1], [ENOSR])], , [#include <errno.h>])
AC_CHECK_DECL(ENOSTR, [AC_DEFINE([HAVE_ENOSTR], [1], [ENOSTR])], , [#include <errno.h>])
AC_CHECK_DECL(ENOSYS, [AC_DEFINE([HAVE_ENOSYS], [1], [ENOSYS])], , [#include <errno.h>])
AC_CHECK_DECL(ENOTBLK, [AC_DEFINE([HAVE_ENOTBLK], [1], [ENOTBLK])], , [#include <errno.h>])
AC_CHECK_DECL(ENOTCONN, [AC_DEFINE([HAVE_ENOTCONN], [1], [ENOTCONN])], , [#include <errno.h>])
AC_CHECK_DECL(ENOTDIR, [AC_DEFINE([HAVE_ENOTDIR], [1], [ENOTDIR])], , [#include <errno.h>])
AC_CHECK_DECL(ENOTEMPTY, [AC_DEFINE([HAVE_ENOTEMPTY], [1], [ENOTEMPTY])], , [#include <errno.h>])
AC_CHECK_DECL(ENOTSOCK, [AC_DEFINE([HAVE_ENOTSOCK], [1], [ENOTSOCK])], , [#include <errno.h>])
AC_CHECK_DECL(ENOTSUP, [AC_DEFINE([HAVE_ENOTSUP], [1], [ENOTSUP])], , [#include <errno.h>])
AC_CHECK_DECL(ENOTTY, [AC_DEFINE([HAVE_ENOTTY], [1], [ENOTTY])], , [#include <errno.h>])
AC_CHECK_DECL(ENOTUNIQ, [AC_DEFINE([HAVE_ENOTUNIQ], [1], [ENOTUNIQ])], , [#include <errno.h>])
AC_CHECK_DECL(ENXIO, [AC_DEFINE([HAVE_ENXIO], [1], [ENXIO])], , [#include <errno.h>])
AC_CHECK_DECL(EOPNOTSUPP, [AC_DEFINE([HAVE_EOPNOTSUPP], [1], [EOPNOTSUPP])], , [#include <errno.h>])
AC_CHECK_DECL(EOVERFLOW, [AC_DEFINE([HAVE_EOVERFLOW], [1], [EOVERFLOW])], , [#include <errno.h>])
AC_CHECK_DECL(EPERM, [AC_DEFINE([HAVE_EPERM], [1], [EPERM])], , [#include <errno.h>])
AC_CHECK_DECL(EPFNOSUPPORT, [AC_DEFINE([HAVE_EPFNOSUPPORT], [1], [EPFNOSUPPORT])], , [#include <errno.h>])
AC_CHECK_DECL(EPIPE, [AC_DEFINE([HAVE_EPIPE], [1], [EPIPE])], , [#include <errno.h>])
AC_CHECK_DECL(EPROTO, [AC_DEFINE([HAVE_EPROTO], [1], [EPROTO])], , [#include <errno.h>])
AC_CHECK_DECL(EPROTONOSUPPORT, [AC_DEFINE([HAVE_EPROTONOSUPPORT], [1], [EPROTONOSUPPORT])], , [#include <errno.h>])
AC_CHECK_DECL(EPROTOTYPE, [AC_DEFINE([HAVE_EPROTOTYPE], [1], [EPROTOTYPE])], , [#include <errno.h>])
AC_CHECK_DECL(ERANGE, [AC_DEFINE([HAVE_ERANGE], [1], [ERANGE])], , [#include <errno.h>])
AC_CHECK_DECL(EREMCHG, [AC_DEFINE([HAVE_EREMCHG], [1], [EREMCHG])], , [#include <errno.h>])
AC_CHECK_DECL(EREMOTE, [AC_DEFINE([HAVE_EREMOTE], [1], [EREMOTE])], , [#include <errno.h>])
AC_CHECK_DECL(EREMOTEIO, [AC_DEFINE([HAVE_EREMOTEIO], [1], [EREMOTEIO])], , [#include <errno.h>])
AC_CHECK_DECL(ERESTART, [AC_DEFINE([HAVE_ERESTART], [1], [ERESTART])], , [#include <errno.h>])
AC_CHECK_DECL(EROFS, [AC_DEFINE([HAVE_EROFS], [1], [EROFS])], , [#include <errno.h>])
AC_CHECK_DECL(ESHUTDOWN, [AC_DEFINE([HAVE_ESHUTDOWN], [1], [ESHUTDOWN])], , [#include <errno.h>])
AC_CHECK_DECL(ESPIPE, [AC_DEFINE([HAVE_ESPIPE], [1], [ESPIPE])], , [#include <errno.h>])
AC_CHECK_DECL(ESOCKTNOSUPPORT, [AC_DEFINE([HAVE_ESOCKTNOSUPPORT], [1], [ESOCKTNOSUPPORT])], , [#include <errno.h>])
AC_CHECK_DECL(ESRCH, [AC_DEFINE([HAVE_ESRCH], [1], [ESRCH])], , [#include <errno.h>])
AC_CHECK_DECL(ESTALE, [AC_DEFINE([HAVE_ESTALE], [1], [ESTALE])], , [#include <errno.h>])
AC_CHECK_DECL(ESTRPIPE, [AC_DEFINE([HAVE_ESTRPIPE], [1], [ESTRPIPE])], , [#include <errno.h>])
AC_CHECK_DECL(ETIME, [AC_DEFINE([HAVE_ETIME], [1], [ETIME])], , [#include <errno.h>])
AC_CHECK_DECL(ETIMEDOUT, [AC_DEFINE([HAVE_ETIMEDOUT], [1], [ETIMEDOUT])], , [#include <errno.h>])
AC_CHECK_DECL(ETXTBSY, [AC_DEFINE([HAVE_ETXTBSY], [1], [ETXTBSY])], , [#include <errno.h>])
AC_CHECK_DECL(EUCLEAN, [AC_DEFINE([HAVE_EUCLEAN], [1], [EUCLEAN])], , [#include <errno.h>])
AC_CHECK_DECL(EUNATCH, [AC_DEFINE([HAVE_EUNATCH], [1], [EUNATCH])], , [#include <errno.h>])
AC_CHECK_DECL(EUSERS, [AC_DEFINE([HAVE_EUSERS], [1], [EUSERS])], , [#include <errno.h>])
AC_CHECK_DECL(EWOULDBLOCK, [AC_DEFINE([HAVE_EWOULDBLOCK], [1], [EWOULDBLOCK])], , [#include <errno.h>])
AC_CHECK_DECL(EXDEV, [AC_DEFINE([HAVE_EXDEV], [1], [EXDEV])], , [#include <errno.h>])
AC_CHECK_DECL(EXFULL, [AC_DEFINE([HAVE_EXFULL], [1], [EXFULL])], , [#include <errno.h>])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gettimeofday isascii memchr memmove memset munmap \
                strchr strrchr strstr strtol dlopen isnan alloca vsnprintf \
                pthread_rwlockattr_setkind_np pthread_rwlockattr_init \
                pthread_rwlockattr_destroy snprintf _isnan])

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
                 ext/Makefile lib/Makefile doc/Makefile])

AC_CANONICAL_HOST
case "${host}" in
*-openbsd*)
    CFLAGS_COMMON="${CFLAGS_COMMON} -pthread"
    LDFLAGS_COMMON="${LDFLAGS_COMMON} -ggdb -export-dynamic"
    TEST_TARGETS="${TEST_TARGETS} test-copying test-mark-sweep test-mark-sweep-compact test-generational"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-copying\$(EXEEXT) yog-mark-sweep\$(EXEEXT) yog-mark-sweep-compact\$(EXEEXT) yog-generational\$(EXEEXT) miniyog\$(EXEEXT)"
    EXTS="concurrent\$(SOEXT)"
    SOEXT=".so"
    ;;
*-mingw32)
    PTHREADS_W32_DIR="`pwd`/pthreads-w32"
    CFLAGS_COMMON="${CFLAGS} -I${PTHREADS_W32_DIR} -DPTW32_STATIC_LIB -DYOG_CORE"
    LDFLAGS_COPYING="${LDFLAGS_COPYING} -Wl,--out-implib,yog-copying.lib"
    LDFLAGS_MINIYOG="${LDFLAGS_MINIYOG} -Wl,--out-implib,miniyog.lib"
    LDFLAGS_COMMON="${LDFLAGS_COMMON} --enable-runtime-pseudo-reloc"
    LIBS="${LIBS} ${PTHREADS_W32_DIR}/libpthreadGC2.a -lwsock32"
    TEST_TARGETS="${TEST_TARGETS} test-copying"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-copying\$(EXEEXT) miniyog\$(EXEEXT)"
    SOEXT=".dll"
    COMMON_SOURCES="icon.rc"
    ;;
*)
    case "${host}" in
    *-linux*)
        CFLAGS_COMMON="${CFLAGS_COMMON} -D_XOPEN_SOURCE=500 -D_BSD_SOURCE"
        ;;
    esac
    LDFLAGS_COMMON="${LDFLAGS_COMMON} -export-dynamic"
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
AC_SUBST(CFLAGS_COMMON)
AC_SUBST(LDFLAGS_COMMON)
AC_SUBST(LDFLAGS_BDW)
AC_SUBST(LDFLAGS_COPYING)
AC_SUBST(LDFLAGS_GENERATIONAL)
AC_SUBST(LDFLAGS_MARK_SWEEP)
AC_SUBST(LDFLAGS_MARK_SWEEP_COMPACT)
AC_SUBST(LDFLAGS_MINIYOG)
AC_SUBST(LIBS)
AC_SUBST(EXTS)
AC_SUBST(SOEXT)
AC_SUBST(COMMON_SOURCES)

AC_CONFIG_SUBDIRS([onig
                   gmp])
AC_OUTPUT

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
