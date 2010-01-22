#if !defined(__YOG_SYSDEPS_H__)
#define __YOG_SYSDEPS_H__

#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#include <stdarg.h>
#include <stdio.h>

#if defined(_MSC_VER)
#   define YogSysdeps_alloca                        _alloca
#   define YogSysdeps_snprintf                      _snprintf
#   define YogSysdeps_vsnprinf(s, size, fmt, ap)    vsprintf(s, fmt, ap)
#else
#   define YogSysdeps_alloca                        alloca
#   define YogSysdeps_snprintf                      snprintf
#   define YogSysdeps_vsnprintf(s, size, fmt, ap)   vsnprintf(s, size, fmt, ap)
#endif

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
