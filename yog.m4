
AC_DEFUN([YOG_CHECK_EXT], [
AC_CHECK_LIB([$1], [$2], [EXT_SUBDIRS="$EXT_SUBDIRS $3"])
])

dnl vim: tabstop=2 shiftwidth=2 expandtab softtabstop=4
