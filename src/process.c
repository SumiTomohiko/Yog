#include "yog/config.h"
#include <unistd.h>
#include "yog/class.h"
#include "yog/handle.h"
#include "yog/object.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Process {
    struct YogBasicObj base;
    pid_t pid;
    YogVal stdin_;
    YogVal stdout_;
    YogVal stderr_;
};

typedef struct Process Process;

static YogVal
init(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* args)
{
    /* TODO */
    return HDL2VAL(self);
}

static YogVal
run(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    /* TODO */
    return YUNDEF;
}

static YogVal
get_stderr(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    /* TODO */
    return YUNDEF;
}

static YogVal
get_stdout(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    /* TODO */
    return YUNDEF;
}

static YogVal
get_stdin(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    /* TODO */
    return YUNDEF;
}

static YogVal
wait(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    /* TODO */
    return YUNDEF;
}

void
YogProcess_define_classes(YogEnv* env, YogHandle* pkg)
{
    YogVM* vm = env->vm;
    YogVal cProcess = YogClass_new(env, "Process", vm->cObject);
    YogHandle* h_cProcess = YogHandle_REGISTER(env, cProcess);
#define DEFINE_METHOD(name, ...) do { \
    YogClass_define_method2(env, HDL2VAL(h_cProcess), HDL2VAL(pkg), (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD("init", init, "args", NULL);
    DEFINE_METHOD("run", run, NULL);
    DEFINE_METHOD("wait", wait, NULL);
#undef DEFINE_METHOD
#define DEFINE_PROP(name, getter, setter) do { \
    YogClass_define_property2(env, h_cProcess, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("stderr", get_stderr, NULL);
    DEFINE_PROP("stdin", get_stdin, NULL);
    DEFINE_PROP("stdout", get_stdout, NULL);
#undef DEFINE_PROP
    vm->cProcess = HDL2VAL(h_cProcess);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
