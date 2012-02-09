#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(Yog, 0.1.0, SumiTomohiko@neko-daisuki.ddo.jp)
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
                  netdb.h sys/socket.h winsock2.h direct.h])

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
YOG_CHECK_ERRNO([E2BIG EACCESS EADDINUSE EADDRNOTAVAIL EAFNOSUPPORT EAGAIN \
                 EALREADY EBADE EBADFD EBADF EBADMSG EBADRQC EBADR EBADSLT \
                 EBUSY ECANCELED ECHILD ECHRNG ECOMM ECONNABORTED ECONNREFUSED \
                 ECONNRESET EDEADLK EDEADLOCK EDESTADDRREQ EDOM EDQUOT EEXIST \
                 EFAULT EFBIG EHOSTDOWN EHOSTUNREACH EIDRM EILSEQ EINPROGRESS \
                 EINTR EINVAL EIO EISCONN EISDIR EISNAM EKEYEXPIRED \
                 EKEYREJECTED EKEYREVOKED EL2HLT EL2NSYNC EL3HLT EL3RST \
                 ELIBACC ELIBBAD ELIBEXEC ELIBMAX ELIBSCN ELOOP EMEDIUMTYPE \
                 EMFILE EMLINK EMSGSIZE EMULTIHOP ENAMETOOLONG ENETDOWN \
                 ENETRESET ENETUNREACH ENFILE ENOBUFS ENODATA ENODEV ENOENT \
                 ENOEXEC ENOKEY ENOLCK ENOLINK ENOMEDIUM ENOMEM ENOMSG ENONET \
                 ENOPKG ENOPROTOOPT ENOSPC ENOSR ENOSTR ENOSYS ENOTBLK \
                 ENOTCONN ENOTDIR ENOTEMPTY ENOTSOCK ENOTSUP ENOTTY ENOTUNIQ \
                 ENXIO EOPNOTSUPP EOVERFLOW EPERM EPFNOSUPPORT EPIPE \
                 EPROTONOSUPPORT EPROTOTYPE EPROTO ERANGE EREMCHG EREMOTEIO \
                 EREMOTE ERESTART EROFS ESHUTDOWN ESOCKTNOSUPPORT ESPIPE ESRCH \
                 ESTALE ESTRPIPE ETIMEDOUT ETIME ETXTBSY EUCLEAN EUNATCH \
                 EUSERS EWOULDBLOCK EXDEV EXFULL])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gettimeofday isascii memchr memmove memset munmap \
                strchr strrchr strstr strtol dlopen isnan alloca vsnprintf \
                pthread_rwlockattr_setkind_np pthread_rwlockattr_init \
                pthread_rwlockattr_destroy snprintf _isnan _mkdir getaddrinfo \
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

AC_CONFIG_FILES([Makefile bin/Makefile doc/Makefile doc/index.html ext/Makefile
                 ext/Makefile.common ext/concurrent/Makefile ext/uname/Makefile
                 ext/yaml/Makefile ext/zip/Makefile lib/Makefile lib/c/Makefile
                 lib/sdl/Makefile src/Makefile tests/Makefile
                 tools/lemon/Makefile vim/Makefile vim/ftdetect/Makefile
                 vim/ftplugin/Makefile])

AC_CANONICAL_HOST
case "${host}" in
*-openbsd*)
    CFLAGS="${CFLAGS} -pthread"
    LDFLAGS_COMMON="${LDFLAGS_COMMON} -ggdb -export-dynamic"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-copying\$(EXEEXT) yog-mark-sweep\$(EXEEXT) yog-mark-sweep-compact\$(EXEEXT) yog\$(EXEEXT)"
    SOEXT="so"
    EXT_SUBDIRS="concurrent"
    ;;
*)
    case "${host}" in
    *-linux*)
        CFLAGS="${CFLAGS} -D_XOPEN_SOURCE=600 -D_BSD_SOURCE"
        ;;
    esac
    LDFLAGS_COMMON="${LDFLAGS_COMMON} -export-dynamic"
    BIN_PROGRAMS="${BIN_PROGRAMS} yog-copying\$(EXEEXT) yog-mark-sweep\$(EXEEXT) yog-mark-sweep-compact\$(EXEEXT) yog\$(EXEEXT)"
    SOEXT="so"
    EXT_SUBDIRS="concurrent"
    ;;
esac

YOG_CHECK_EXT([syck], [syck_new_parser], [yaml])
YOG_CHECK_EXT([zip], [zip_open], [zip])

AC_SUBST(BIN_PROGRAMS)
AC_SUBST(PACKAGE_VERSION)
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS_COMMON)
AC_SUBST(LDFLAGS_COPYING)
AC_SUBST(LDFLAGS_GENERATIONAL)
AC_SUBST(LDFLAGS_MARK_SWEEP)
AC_SUBST(LDFLAGS_MARK_SWEEP_COMPACT)
AC_SUBST(LIBS)
AC_SUBST(SOEXT)
AC_SUBST(IMPORT_LIB)
AC_SUBST(EXT_SUBDIRS)

AC_CONFIG_SUBDIRS([corgi gmp libffi tools/swig])

AC_OUTPUT

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
