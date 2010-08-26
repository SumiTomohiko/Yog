#if !defined(YOG_COMPAT_H_INCLUDED)
#define YOG_COMPAT_H_INCLUDED

#include "yog/config.h"
#include <stdarg.h>

#if !defined(va_copy)
#   define va_copy(aq, ap) aq = ap /* probably */
#endif

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
