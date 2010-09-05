#include "yog/config.h"
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

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
