#if !defined(__YOG_SYSDEPS_H__)
#define __YOG_SYSDEPS_H__

#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
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
#if defined(HAVE_WINDOWS_H)
#   include <windows.h>
#endif

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
#if defined(HAVE_MKDIR)
#   define YogSysdeps_mkdir(path)   mkdir(path, 0755)
#else
#   define YogSysdeps_mkdir(path)   _mkdir(path)
#endif
#if defined(HAVE_BZERO)
#   define YogSysdeps_bzero(s, n)   bzero(s, n)
#else
#   define YogSysdeps_bzero(s, n)   memset(s, 0, n)
#endif
#if defined(HAVE_DLOPEN)
#   define YogSysdeps_open_lib(path)    dlopen((path), RTLD_LAZY)
#   define LIB_HANDLE                   void*
#else
#   define YogSysdeps_open_lib(path)    LoadLibrary((path))
#   define LIB_HANDLE                   HINSTANCE
#endif

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
