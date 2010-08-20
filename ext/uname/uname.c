#include <errno.h>
#include <sys/utsname.h>
#include "yog/error.h"
#include "yog/handle.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/yog.h"

static void
do_uname(YogEnv* env, struct utsname* name)
{
    if (uname(name) == 0) {
        return;
    }
    YogError_raise_sys_err(env, errno, YNIL);
}

#define DEFINE_FUNC_BODY(name) \
    static YogVal \
    name(YogEnv* env, YogHandle* self, YogHandle* pkg) \
    { \
        struct utsname __name__; \
        do_uname(env, &__name__); \
        return YogString_from_str(env, __name__.name); \
    }

DEFINE_FUNC_BODY(release)
DEFINE_FUNC_BODY(sysname)
DEFINE_FUNC_BODY(nodename)
DEFINE_FUNC_BODY(version)
DEFINE_FUNC_BODY(machine)

YogVal
YogInit_uname(YogEnv* env)
{
    YogVal pkg = YogPackage_new(env);
    YogHandle* h = YogHandle_REGISTER(env, pkg);
#define DEFINE_FUNC(name) do { \
    YogPackage_define_function2(env, h, #name, name, NULL); \
} while (0)
    DEFINE_FUNC(release);
    DEFINE_FUNC(sysname);
    DEFINE_FUNC(nodename);
    DEFINE_FUNC(version);
    DEFINE_FUNC(machine);
#undef DEFINE_FUNC

    return HDL2VAL(h);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
