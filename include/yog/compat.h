#if !defined(__YOG_COMPAT_H__)
#define __YOG_COMPAT_H__

#include "yog/config.h"
#include <stdarg.h>

#if !defined(va_copy)
#   define va_copy(aq, ap) aq = ap /* probably */
#endif

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
