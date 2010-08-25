
AC_DEFUN([YOG_CHECK_EXT], [
  AC_CHECK_LIB([$1], [$2], [EXT_SUBDIRS="$EXT_SUBDIRS $3"])
])

m4_define([CHECK_ERRNO],
[m4_foreach_w([ERRNO], [$1],
  [AH_TEMPLATE(AS_TR_CPP([HAVE_]m4_defn([ERRNO])),
    [Define to 1 if you have the <]m4_defn([ERRNO])[> header file.])])])

AC_DEFUN([YOG_CHECK_ERRNO], [
CHECK_ERRNO([$1])
for ac_errno in $1
do
  AC_CHECK_DECL($ac_errno,
    [AC_DEFINE([HAVE_$ac_errno], [1], [$ac_errno])],
    ,
    [#include <errno.h>])dnl
done
])

dnl vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
