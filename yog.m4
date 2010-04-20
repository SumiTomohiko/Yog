
AC_DEFUN([YOG_CHECK_EXT], [
    AC_CHECK_LIB([$1], [$2], [EXT_SUBDIRS="$EXT_SUBDIRS $3"])
])

AC_DEFUN([YOG_CHECK_ERRNO], [
    AC_CHECK_DECL($1, [AC_DEFINE([HAVE_$1], [1], [$1])], , [#include <errno.h>])
])

dnl vim: tabstop=2 shiftwidth=2 expandtab softtabstop=4
