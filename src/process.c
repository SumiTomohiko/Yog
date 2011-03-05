#include "yog/config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/file.h"
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
}

#define R 0
#define W 1

#define CLOSE_PIPE(name) do { \
    close(name[R]); \
    close(name[W]); \
} while (0)

static void
exec_child(YogEnv* env, YogHandle* self, int pipe_stdin[2], int pipe_stdout[2], int pipe_stderr[2])
{
    YogVal args = HDL_AS(Process, self)->args;
    if (!IS_PTR(args) || (BASIC_OBJ_TYPE(args) != TYPE_ARRAY)) {
        YogError_raise_TypeError(env, "Arguments must be Array, not %C", args);
    }
    uint_t size = YogArray_size(env, args);
    YogHandle* h = VAL2HDL(env, args);
    YogHandle* bins[size];
    uint_t i;
    for (i = 0; i < size; i++) {
        YogVal a = YogArray_at(env, HDL2VAL(h), i);
        check_string(env, a);
        YogVal bin = YogString_to_bin_in_default_encoding(env, VAL2HDL(env, a));
        bins[i] = VAL2HDL(env, bin);
    }
    char* argv[size + 1];
    for (i = 0; i < size; i++) {
        argv[i] = BINARY_CSTR(HDL2VAL(bins[i]));
    }
    argv[size] = NULL;

    close(pipe_stdin[W]);
    if (dup2(pipe_stdin[R], 0) == -1) {
        close(pipe_stdin[R]);
        CLOSE_PIPE(pipe_stdout);
        CLOSE_PIPE(pipe_stderr);
        YogError_raise_sys_err(env, errno, YUNDEF);
    }
    close(pipe_stdin[R]);
    close(pipe_stdout[R]);
    if (dup2(pipe_stdout[W], 1) == -1) {
        close(pipe_stdout[W]);
        CLOSE_PIPE(pipe_stderr);
        YogError_raise_sys_err(env, errno, YUNDEF);
    }
    close(pipe_stdout[W]);
    close(pipe_stderr[R]);
    if (dup2(pipe_stderr[W], 2) == -1) {
        close(pipe_stderr[W]);
        YogError_raise_sys_err(env, errno, YUNDEF);
    }
    close(pipe_stderr[W]);

    execv(argv[0], argv);
    /* NOTREACHED */
    YogError_raise_sys_err(env, errno, args);
}

static void
create_pipe(YogEnv* env, int pipefd[2])
{
    if (pipe(pipefd) == 0) {
        return;
    }
    YogError_raise_sys_err(env, errno, YUNDEF);
}

static YogVal
run(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
#define CREATE_PIPE(name) \
    int name[2]; \
    create_pipe(env, name)
    CREATE_PIPE(pipe_stdin);
    CREATE_PIPE(pipe_stdout);
    CREATE_PIPE(pipe_stderr);
#undef CREATE_PIPE
    pid_t pid = fork();
    if (pid == -1) {
        CLOSE_PIPE(pipe_stdin);
        CLOSE_PIPE(pipe_stdout);
        CLOSE_PIPE(pipe_stderr);
        YogError_raise_sys_err(env, errno, YUNDEF);
    }
    if (pid == 0) {
        exec_child(env, self, pipe_stdin, pipe_stdout, pipe_stderr);
    }

    HDL_AS(Process, self)->pid = pid;
    close(pipe_stdin[R]);
    close(pipe_stdout[W]);
    close(pipe_stderr[W]);
    YogHandle* enc = YogHandle_REGISTER(env, YogEncoding_get_ascii(env));
#define CREATE_FILE(fd, mode, name) do { \
    YogVal fp = YogFile_new(env); \
    PTR_AS(YogFile, fp)->fp = fdopen((fd), (mode)); \
    YogGC_UPDATE_PTR(env, PTR_AS(YogFile, fp), encoding, HDL2VAL(enc)); \
    YogGC_UPDATE_PTR(env, HDL_AS(Process, self), name, fp); \
} while (0)
    CREATE_FILE(pipe_stdin[W], "w", stdin_);
    CREATE_FILE(pipe_stdout[R], "r", stdout_);
    CREATE_FILE(pipe_stderr[R], "r", stderr_);
#undef CREATE_FILE

    return HDL2VAL(self);
}

static YogVal
get_stderr(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    return HDL_AS(Process, self)->stderr_;
}

static YogVal
get_stdout(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    return HDL_AS(Process, self)->stdout_;
}

static YogVal
get_stdin(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    return HDL_AS(Process, self)->stdin_;
}

static YogVal
do_waitpid(YogEnv* env, YogHandle* self, int options)
{
    CHECK_SELF_TYPE(env, self);
    int status;
    switch (waitpid(HDL_AS(Process, self)->pid, &status, options)) {
    case -1:
        YogError_raise_sys_err(env, errno, YUNDEF);
        /**
         * gcc claims error when the following statement doesn't exist.
         */
        return YUNDEF;
    case 0:
        YOG_ASSERT2(env, options & WNOHANG);
        return YNIL;
    default:
        break;
    }
    if (!WIFEXITED(status)) {
        return YNIL;
    }
    return YogVal_from_int(env, WEXITSTATUS(status));
}

static YogVal
poll(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    return do_waitpid(env, self, WNOHANG);
}

static YogVal
wait_(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    return do_waitpid(env, self, 0);
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
    DEFINE_METHOD("poll", poll, NULL);
    DEFINE_METHOD("run", run, NULL);
    DEFINE_METHOD("wait", wait_, NULL);
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
