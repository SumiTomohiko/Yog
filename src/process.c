#include "yog/config.h"
#include <unistd.h>
#include "yog/class.h"
#include "yog/gc.h"
#include "yog/handle.h"
#include "yog/object.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Process {
    struct YogBasicObj base;
    pid_t pid;
    YogVal args;
    YogVal stdin_;
    YogVal stdout_;
    YogVal stderr_;
};

typedef struct Process Process;

#define TYPE_PROCESS TO_TYPE(alloc)

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    Process* proc = (Process*)ptr;
#define KEEP(name) YogGC_KEEP(env, proc, name, keeper, heap)
    KEEP(args);
    KEEP(stdin_);
    KEEP(stdout_);
    KEEP(stderr_);
#undef KEEP
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    YogHandle* h = YogHandle_REGISTER(env, klass);
    YogVal obj = ALLOC_OBJ(env, keep_children, NULL, Process);
    YogBasicObj_init(env, obj, TYPE_PROCESS, 0, HDL2VAL(h));
#define INIT(name, val)
    INIT(pid, 0);
    INIT(args, YNIL);
    INIT(stdin_, YNIL);
    INIT(stdout_, YNIL);
    INIT(stderr_, YNIL);
#undef INIT
    return obj;
}

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
    YogClass_define_allocator(env, HDL2VAL(h_cProcess), alloc);
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
