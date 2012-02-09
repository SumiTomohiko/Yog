#if !defined(YOG_SYSDEPS_H_INCLUDED)
#define YOG_SYSDEPS_H_INCLUDED

#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#if defined(HAVE_DIRECT_H)
#   include <direct.h>
#endif
#if defined(HAVE_DLFCN_H)
#   include <dlfcn.h>
#endif
#include <errno.h>
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(HAVE__ALLOCA)
#   define YogSysdeps_alloca    _alloca
#else
#   define YogSysdeps_alloca    alloca
#endif
#if defined(HAVE__ISNAN)
#   define YogSysdeps_isnan     _isnan
#else
#   define YogSysdeps_isnan     isnan
#endif
#if defined(HAVE__SNPRINTF)
#   define YogSysdeps_snprintf  _snprintf
#else
#   define YogSysdeps_snprintf  snprintf
#endif
#if defined(HAVE_VSNPRINTF)
#   define YogSysdeps_vsnprintf(s, size, fmt, ap)   vsnprintf(s, size, fmt, ap)
#else
#   define YogSysdeps_vsnprintf(s, size, fmt, ap)   vsprintf(s, fmt, ap)
#endif
#define YogSysdeps_mkdir(path)   mkdir(path, 0755)
#if defined(HAVE_BZERO)
#   define YogSysdeps_bzero(s, n)   bzero(s, n)
#else
#   define YogSysdeps_bzero(s, n)   memset(s, 0, n)
#endif
#if defined(HAVE_DLOPEN)
#   define LIB_HANDLE                   void*
#   define YogSysdeps_open_lib(path)    dlopen((path), RTLD_LAZY)
#   define YogSysdeps_get_proc(handle, name) \
                                        dlsym((handle), (name))
#   define YogSysdeps_dlerror()         dlerror()
#else
#   define LIB_HANDLE                   HINSTANCE
#   define YogSysdeps_open_lib(path)    LoadLibrary((path))
#   define YogSysdeps_get_proc(handle, name) \
                                        GetProcAddress((handle), (name))
#   define YogSysdeps_dlerror()
#endif

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
