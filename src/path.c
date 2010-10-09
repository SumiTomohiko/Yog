#include "yog/config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "yog/error.h"
#include "yog/handle.h"
#include "yog/string.h"
#include "yog/yog.h"

static YogHandle*
append_separator(YogEnv* env, YogHandle* self)
{
    YogHandle* path = VAL2HDL(env, YogString_clone(env, HDL2VAL(self)));
    YogString_push(env, HDL2VAL(path), PATH_SEPARATOR);
    return path;
}

YogHandle*
YogPath_join2(YogEnv* env, YogHandle* self, YogHandle* name)
{
    YogHandle* path = append_separator(env, self);
    YogString_append(env, HDL2VAL(path), HDL2VAL(name));
    return path;
}

YogHandle*
YogPath_join(YogEnv* env, YogHandle* self, const char* name)
{
    YogHandle* path = append_separator(env, self);
    YogString_append_string(env, HDL2VAL(path), name);
    return path;
}

YogHandle*
YogPath_dirname(YogEnv* env, YogHandle* self)
{
    uint_t pos = YogString_strrchr(env, HDL2VAL(self), PATH_SEPARATOR);
    if (pos < 0) {
        return VAL2HDL(env, YogString_from_string(env, "."));
    }
    return VAL2HDL(env, YogString_slice(env, self, 0, pos));
}

YogVal
YogPath_getcwd(YogEnv* env)
{
    size_t size = 1;
    char* buf;
    char* dir;
    do {
        size *= 1024;
        buf = alloca(sizeof(char) * size);
    } while (((dir = getcwd(buf, size)) == NULL) && (errno == ERANGE));
    if (dir == NULL) {
        YogError_raise_sys_err(env, errno, YNIL);
        /* NOTREACHED */
    }
    return YogString_from_string(env, dir);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
