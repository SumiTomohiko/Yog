#include "yog/config.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "yog/array.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/handle.h"
#include "yog/object.h"
#include "yog/string.h"
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

#define CHECK_SELF_TYPE(env, self) do { \
    YogVal obj = HDL2VAL((self)); \
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_PROCESS)) { \
        YogError_raise_TypeError((env), "self must be Process"); \
    } \
} while (0)

static YogVal
init(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* args)
{
    CHECK_SELF_TYPE(env, self);
    YogGC_UPDATE_PTR(env, HDL_AS(Process, self), args, HDL2VAL(args));
    return HDL2VAL(self);
}

static void
check_string(YogEnv* env, YogVal s)
{
    if (IS_PTR(s) && (BASIC_OBJ_TYPE(s) == TYPE_STRING)) {
        return;
    }
    YogError_raise_TypeError(env, "Argument must be String, not %C", s);
    /* NOTREACHED */
}

static void
exec_child(YogEnv* env, YogHandle* self)
{
    YogVal args = HDL_AS(Process, self)->args;
    if (!IS_PTR(args) || (BASIC_OBJ_TYPE(args) != TYPE_ARRAY)) {
        YogError_raise_TypeError(env, "Arguments must be Array, not %C", args);
        /* NOTREACHED */
    }
    uint_t size = YogArray_size(env, args);
    char* argv[size + 1];
    uint_t i;
    for (i = 0; i < size; i++) {
        YogVal a = YogArray_at(env, args, i);
        check_string(env, a);
        argv[i] = STRING_CSTR(a);
    }
    argv[size] = NULL;
    execv(argv[0], argv);
    YogError_raise_sys_err(env, errno, args);
    /* NOTREACHED */
}

static YogVal
run(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    pid_t pid = fork();
    if (pid == -1) {
        YogError_raise_sys_err(env, errno, YUNDEF);
        /* NOTREACHED */
    }
    if (pid == 0) {
        exec_child(env, self);
        /* NOTREACHED */
    }

    return HDL2VAL(self);
}

static YogVal
get_stderr(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    /* TODO */
    return YUNDEF;
}

static YogVal
get_stdout(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    /* TODO */
    return YUNDEF;
}

static YogVal
get_stdin(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    /* TODO */
    return YUNDEF;
}

static YogVal
wait(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
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
