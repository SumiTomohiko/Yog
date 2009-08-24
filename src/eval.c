#include <setjmp.h>
#include <string.h>
#include "yog/binary.h"
#include "yog/block.h"
#include "yog/code.h"
#include "yog/compile.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/function.h"
#include "yog/misc.h"
#include "yog/module.h"
#include "yog/package.h"
#include "yog/parser.h"
#include "yog/set.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif
#define DUMP_CODE(code)  YogCode_dump(env, code)

#define CUR_FRAME   PTR_AS(YogThread, env->thread)->cur_frame

#define PUSH_FRAME(f)   do { \
    PTR_AS(YogFrame, (f))->prev = CUR_FRAME; \
    CUR_FRAME = (f); \
} while (0)

#define POP_FRAME()     do { \
    CUR_FRAME = PTR_AS(YogFrame, CUR_FRAME)->prev; \
} while (0)

static void
exec_get_attr(YogEnv* env, YogVal obj, ID name)
{
    SAVE_ARG(env, obj);
    YogVal attr = YUNDEF;
    YogVal class_of_obj = YUNDEF;
    YogVal class_of_attr = YUNDEF;
    YogVal class_name = YUNDEF;
    YogVal attr_name = YUNDEF;
    PUSH_LOCALS5(env, attr, class_of_obj, class_of_attr, class_name, attr_name);

    class_of_obj = YogVal_get_class(env, obj);

    if (IS_PTR(obj) && ((PTR_AS(YogBasicObj, obj)->flags & HAS_ATTRS) != 0)) {
        attr = YogObj_get_attr(env, obj, name);
    }
    if (IS_UNDEF(attr)) {
        attr = YogClass_get_attr(env, class_of_obj, name);
    }
    if (IS_UNDEF(attr)) {
        YogVM* vm = env->vm;
        ID id = PTR_AS(YogClass, class_of_obj)->name;
        class_name = YogVM_id2name(env, vm, id);
        attr_name = YogVM_id2name(env, vm, name);
        YogError_raise_AttributeError(env, "'%s' object has no attribute '%s'", STRING_CSTR(class_name), STRING_CSTR(attr_name));
    }
    class_of_attr = YogVal_get_class(env, attr);
    void (*exec)(YogEnv*, YogVal, YogVal, YogVal) = PTR_AS(YogClass, class_of_attr)->exec_get_descr;
    if (exec == NULL) {
        FRAME_PUSH(env, attr);
    }
    else {
        exec(env, attr, obj, class_of_obj);
    }

    RETURN_VOID(env);
}

static YogVal*
get_outer_vars_ptr(YogEnv* env, uint_t level, uint_t index)
{
    YogVal outer_vars = PTR_AS(YogScriptFrame, CUR_FRAME)->outer_vars;
    YOG_ASSERT(env, IS_PTR(outer_vars), "no outer variables");
    uint_t depth = PTR_AS(YogOuterVars, outer_vars)->size;
    YOG_ASSERT(env, level <= depth, "invalid level");
    YogVal vars = PTR_AS(YogOuterVars, outer_vars)->items[level - 1];
    uint_t size = YogValArray_size(env, vars);
    YOG_ASSERT(env, index < size, "invalid index");
    return &PTR_AS(YogValArray, vars)->items[index];
}

YogVal
YogEval_call_method(YogEnv* env, YogVal receiver, const char* method, uint_t argc, YogVal* args)
{
    SAVE_ARG(env, receiver);

    ID id = YogVM_intern(env, env->vm, method);
    YogVal retval = YogEval_call_method_id(env, receiver, id, argc, args);

    RETURN(env, retval);
}

YogVal
YogEval_call_method1(YogEnv* env, YogVal receiver, const char* method, YogVal arg)
{
    SAVE_ARG(env, receiver);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);
    YogVal args[] = { arg };
    PUSH_LOCALSX(env, 1, args);

    retval = YogEval_call_method(env, receiver, method, 1, args);

    RETURN(env, retval);
}

YogVal
YogEval_call_method0(YogEnv* env, YogVal receiver, const char* method)
{
    SAVE_ARG(env, receiver);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogEval_call_method(env, receiver, method, 0, NULL);

    RETURN(env, retval);
}

YogVal
YogEval_call_method2(YogEnv* env, YogVal receiver, const char* method, uint_t argc, YogVal* args, YogVal blockarg)
{
    SAVE_ARGS2(env, receiver, blockarg);

    ID id = YogVM_intern(env, env->vm, method);
    YogVal retval = YogEval_call_method_id2(env, receiver, id, argc, args, blockarg);

    RETURN(env, retval);
}

static YogVal
make_outer_vars(YogEnv* env, uint_t depth)
{
    if (depth == 0) {
        return YNIL;
    }

    SAVE_LOCALS(env);

    YogVal outer_vars = YUNDEF;
    YogVal vars = YUNDEF;
    PUSH_LOCALS2(env, outer_vars, vars);

    outer_vars = YogOuterVars_new(env, depth);
    vars = PTR_AS(YogMethodFrame, CUR_FRAME)->vars;
    PTR_AS(YogOuterVars, outer_vars)->items[0] = vars;
    if (depth == 1) {
        RETURN(env, outer_vars);
    }

    void* dest = &PTR_AS(YogOuterVars, outer_vars)->items[1];
    vars = PTR_AS(YogScriptFrame, CUR_FRAME)->outer_vars;
    YOG_ASSERT(env, IS_PTR(vars), "vars is not pointer");
    void* src = &PTR_AS(YogOuterVars, vars)->items[0];
    size_t unit = sizeof(PTR_AS(YogOuterVars, vars)->items[0]);
    memcpy(dest, src, unit * (depth - 1));

    RETURN(env, outer_vars);
}

static void
setup_script_function(YogEnv* env, YogVal f, YogVal code)
{
    SAVE_ARGS2(env, f, code);

    PTR_AS(YogFunction, f)->code = code;
    PTR_AS(YogFunction, f)->globals = PTR_AS(YogScriptFrame, CUR_FRAME)->globals;

    uint_t outer_size = PTR_AS(YogCode, code)->outer_size;
    YogVal outer_vars = make_outer_vars(env, outer_size);
    PTR_AS(YogFunction, f)->outer_vars = outer_vars;

    RETURN_VOID(env);
}

static void
setup_script_frame(YogEnv* env, YogVal frame, YogVal code)
{
    SAVE_ARGS2(env, frame, code);

#if 0
    printf("%s:%d setup_script_frame(env=%p, frame=%p, code=%p)\n", __FILE__, __LINE__, env, frame, code);
    YogCode_dump(env, code);
#endif

    uint_t stack_size = PTR_AS(YogCode, code)->stack_size;
    YogVal stack = YogValArray_new(env, stack_size);

    SCRIPT_FRAME(frame)->pc = 0;
    SCRIPT_FRAME(frame)->code = code;
    SCRIPT_FRAME(frame)->stack = stack;

    RETURN_VOID(env);
}

static YogVal
lookup_builtins(YogEnv* env, ID name)
{
    YogVal builtins_name = ID2VAL(YogVM_intern(env, env->vm, "builtins"));
    YogVal builtins = YUNDEF;
    YogVM* vm = env->vm;
    if (!YogTable_lookup(env, vm->pkgs, builtins_name, &builtins)) {
        YOG_ASSERT(env, FALSE, "Can't find builtins package.");
    }

    YogPackage* pkg = PTR_AS(YogPackage, builtins);
    YogVal key = ID2VAL(name);
    YogVal val = YUNDEF;
    if (!YogTable_lookup(env, YOGOBJ(pkg)->attrs, key, &val)) {
        return YUNDEF;
    }

    return val;
}

static BOOL
find_exception_table_entry(YogEnv* env, YogVal code, int status, uint_t pc, uint_t* target)
{
    uint_t i;
    for (i = 0; i < PTR_AS(YogCode, code)->exc_tbl_size; i++) {
        YogVal exc_tbl = PTR_AS(YogCode, code)->exc_tbl;
        YogExceptionTableEntry* entry = &PTR_AS(YogExceptionTable, exc_tbl)->items[i];
        if ((entry->from < pc) && (pc <= entry->to) && (entry->status == status)) {
            if (target != NULL) {
                *target = entry->target;
            }
            return TRUE;
        }
    }

    return FALSE;
}

static void
detect_orphan(YogEnv* env, int status, YogVal target_frame)
{
    YogVal thread = env->thread;
    YogVal frame = PTR_AS(YogThread, thread)->cur_frame;
    while (IS_PTR(frame = PTR_AS(YogFrame, frame)->prev)) {
        YogFrameType type = PTR_AS(YogFrame, frame)->type;
        switch (type) {
        case FRAME_METHOD:
        case FRAME_NAME:
            if (frame != target_frame) {
                continue;
            }
            if (find_exception_table_entry(env, PTR_AS(YogScriptFrame, frame)->code, status, PTR_AS(YogScriptFrame, frame)->pc, NULL)) {
                return;
            }
            break;
        case FRAME_C:
        case FRAME_FINISH:
            break;
        default:
            YOG_BUG(env, "invalid frame type (0x%x)", type);
            break;
        }
    }
    const char* stmt = NULL;
    switch (status) {
    case JMP_RETURN:
        stmt = "return";
        break;
    case JMP_BREAK:
        stmt = "break";
        break;
    case JMP_RAISE:
    default:
        YOG_BUG(env, "invalid status (0x%x)", status);
        break;
    }
    YogError_raise_LocalJumpError(env, "frame to %s is lost", stmt);
}

#if 0
static void
dump_frame(YogEnv* env)
{
    TRACE("----------------");
    YogVal thread = env->thread;
    YogVal frame = PTR_AS(YogThread, thread)->cur_frame;
    while (IS_PTR(frame)) {
        const char* type;
        switch (PTR_AS(YogFrame, frame)->type) {
        case FRAME_METHOD:
            type = "FRAME_METHOD";
            break;
        case FRAME_NAME:
            type = "FRAME_NAME";
            break;
        case FRAME_C:
            type = "FRAME_C";
            break;
        case FRAME_FINISH:
            type = "FRAME_FINISH";
            break;
        default:
            type = "unknown";
            break;
        }
        TRACE("frame=0x%08x, type=%s", frame, type);
        frame = PTR_AS(YogFrame, frame)->prev;
    }
    TRACE("----------------");
}
#endif

static void
long_jump_current_frame(YogEnv* env)
{
    YogVal thread = env->thread;
    YogVal frame = PTR_AS(YogThread, thread)->cur_frame;
    while (IS_PTR(frame = PTR_AS(YogFrame, frame)->prev)) {
        YogFrameType type = PTR_AS(YogFrame, frame)->type;
        switch (type) {
        case FRAME_METHOD:
        case FRAME_NAME:
            break;
        case FRAME_C:
            PTR_AS(YogThread, thread)->cur_frame = frame;
            return;
            break;
        case FRAME_FINISH:
            PTR_AS(YogThread, thread)->cur_frame = PTR_AS(YogFrame, frame)->prev;
            return;
            break;
        default:
            YOG_BUG(env, "invalid frame type (0x%x)", type);
            break;
        }
    }

    YOG_BUG(env, "can't long jump current frame");
}

static void
long_jump(YogEnv* env, YogVal jmp_val, int_t status, YogVal target_frame)
{
    detect_orphan(env, status, target_frame);

    YogVal thread = env->thread;
    PTR_AS(YogThread, thread)->jmp_val = jmp_val;
    PTR_AS(YogThread, thread)->frame_to_long_jump = target_frame;
    longjmp(PTR_AS(YogThread, thread)->jmp_buf_list->buf, status);

    /* NOTREACHED */
}

static void
jump_to_prev_buf(YogEnv* env, int status)
{
    long_jump_current_frame(env);

    POP_JMPBUF(env);
    YogJmpBuf* list = PTR_AS(YogThread, env->thread)->jmp_buf_list;
    if (list != NULL) {
        longjmp(list->buf, status);
    }
}

YogVal
YogEval_mainloop(YogEnv* env)
{
    SAVE_LOCALS(env);

#define PC          (SCRIPT_FRAME(CUR_FRAME)->pc)
#undef CODE
#define CODE        PTR_AS(YogCode, SCRIPT_FRAME(CUR_FRAME)->code)
    YogJmpBuf jmpbuf;
    PUSH_JMPBUF(env->thread, jmpbuf);
    SAVE_CURRENT_STAT(env, mainloop);

#define PUSH(val)   YogScriptFrame_push_stack(env, SCRIPT_FRAME(CUR_FRAME), val)
    int_t status;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
    }
    else {
        YogVal thread = env->thread;
        PTR_AS(YogThread, thread)->jmp_buf_list = mainloop_jmpbuf;
        PTR_AS(YogThread, thread)->locals = mainloop_locals;
        YogVal frame = PTR_AS(YogThread, thread)->cur_frame;
        if (PTR_AS(YogFrame, frame)->type == FRAME_C) {
            do {
                frame = PTR_AS(YogFrame, frame)->prev;
            } while (PTR_AS(YogFrame, frame)->type == FRAME_C);
            PTR_AS(YogThread, thread)->cur_frame = frame;
        }

        switch (status) {
        case JMP_RAISE:
            {
                BOOL found = FALSE;
                if (PTR_AS(YogFrame, CUR_FRAME)->type != FRAME_C) {
                    uint_t target = 0;
                    found = find_exception_table_entry(env, (YogVal)CODE, status, PC, &target);
                    if (found) {
                        PC = target;
                    }
                }
                if (!found) {
                    jump_to_prev_buf(env, status);
                    YogError_print_stacktrace(env);
                    RETURN(env, INT2VAL(-1));
                }
            }
            break;
        case JMP_RETURN:
        case JMP_BREAK:
            {
                YogVal thread = env->thread;
                YogVal frame = PTR_AS(YogThread, thread)->cur_frame;
                while (IS_PTR(frame)) {
                    BOOL found = FALSE;
                    YogFrameType type = PTR_AS(YogFrame, frame)->type;
                    switch (type) {
                    case FRAME_C:
                        jump_to_prev_buf(env, status);
                        YOG_BUG(env, "no destination to longjmp");
                        break;
                    case FRAME_METHOD:
                    case FRAME_NAME:
                        {
                            if (frame == PTR_AS(YogThread, thread)->frame_to_long_jump) {
                                uint_t target = 0;
                                if (!find_exception_table_entry(env, PTR_AS(YogScriptFrame, frame)->code, status, PTR_AS(YogScriptFrame, frame)->pc, &target)) {
                                    YOG_BUG(env, "can't find exception table entry");
                                }
                                CUR_FRAME = frame;
                                PC = target;
                                PUSH(PTR_AS(YogThread, env->thread)->jmp_val);
                                found = TRUE;
                            }
                        }
                        break;
                    case FRAME_FINISH:
                        break;
                    default:
                        YOG_BUG(env, "invalid frame type (0x%x)", type);
                        break;
                    }
                    if (found) {
                        break;
                    }

                    frame = PTR_AS(YogFrame, frame)->prev;
                }
                if (!IS_PTR(frame)) {
                    const char* stmt = NULL;
                    switch (status) {
                    case JMP_RETURN:
                        stmt = "return";
                        break;
                    case JMP_BREAK:
                        stmt = "break";
                        break;
                    case JMP_RAISE:
                    default:
                        YOG_BUG(env, "invalid status (0x%x)", status);
                        break;
                    }
                    YogError_raise_LocalJumpError(env, "frame to %s is lost", stmt);
                }
            }
            break;
        default:
            YOG_BUG(env, "invalid status (0x%x)", status);
            break;
        }
    }

#if 0
    YogCode_dump(env, (YogVal)(CODE));
#endif

    while (PC < PTR_AS(YogByteArray, CODE->insts)->size) {
#define POP()           (YogScriptFrame_pop_stack(env, SCRIPT_FRAME(CUR_FRAME)))
#define CONSTS(index)   (YogValArray_at(env, CODE->consts, index))
#define THREAD          (env->thread)
#define JUMP(m)         PC = m;
#define POP_ARGS(args, kwargs, blockarg, vararg, varkwarg) \
    YogVal varkwarg = YUNDEF; \
    YogVal vararg = YUNDEF; \
    YogVal blockarg = YUNDEF; \
    YogVal kwargs[2 * (kwargc)]; \
    YogVal args[(argc)]; \
    do { \
        uint_t i = 0; \
        for (i = 0; i < 2 * (kwargc); i++) { \
            kwargs[i] = YUNDEF; \
        } \
        for (i = 0; i < (argc); i++) { \
            args[i] = YUNDEF; \
        } \
    } while (0); \
    PUSH_LOCALS3(env, varkwarg, vararg, blockarg); \
    PUSH_LOCALSX(env, 2 * (kwargc), kwargs); \
    PUSH_LOCALSX(env, (argc), args); \
\
    if (varkwargc == 1) { \
        varkwarg = POP(); \
    } \
\
    if (varargc == 1) { \
        vararg = POP(); \
    } \
\
    if (blockargc == 1) { \
        blockarg = POP(); \
    } \
\
    do { \
        uint_t i; \
        for (i = kwargc; 0 < i; i--) { \
            kwargs[2 * i - 1] = POP(); \
            kwargs[2 * i - 2] = POP(); \
        } \
\
        for (i = argc; 0 < i; i--) { \
            args[i - 1] = POP(); \
        } \
    } while (0)
        OpCode op = PTR_AS(YogByteArray, CODE->insts)->items[PC];

#if 0
        do {
            const char* opname = YogCode_get_op_name(op);
            TRACE("%p: PC=%u, op=%s", env, PC, opname);
        } while (0);
#endif
#if 0
        do {
            TRACE("---------------- dump of variables ----------------");
            YogVal cur_frame = PTR_AS(YogThread, env->thread)->cur_frame;
            if (PTR_AS(YogFrame, cur_frame)->type == FRAME_METHOD) {
                YogVal code = PTR_AS(YogScriptFrame, cur_frame)->code;
                ID* names = PTR_AS(YogCode, code)->local_vars_names;
                YogVal vars = PTR_AS(YogMethodFrame, cur_frame)->vars;
                uint_t size = PTR_AS(YogValArray, vars)->size;
                uint_t i;
                for (i = 0; i < size; i++) {
                    YogVal s = YogVM_id2name(env, env->vm, names[i]);
                    TRACE("%u: %s:", i, STRING_CSTR(s));
                    YogVal cur_frame = PTR_AS(YogThread, env->thread)->cur_frame;
                    YogVal vars = PTR_AS(YogMethodFrame, cur_frame)->vars;
                    YogVal_print(env, PTR_AS(YogValArray, vars)->items[i]);
                }
            }
            const char* opname = YogCode_get_op_name(op);
            TRACE("%p: PC=%u, op=%s", env, PC, opname);
        } while (0);
#endif
#if 0
        do {
            printf("%p: ---------------- dump of stack ----------------\n", env);
            YogVal stack = SCRIPT_FRAME(CUR_FRAME)->stack;
            uint_t stack_size = SCRIPT_FRAME(CUR_FRAME)->stack_size;
            if (0 < stack_size) {
                uint_t i;
                for (i = stack_size; 0 < i; i--) {
                    YogVal_print(env, PTR_AS(YogValArray, stack)->items[i - 1]);
                }
            }
            else {
                printf("%p: stack is empty.\n", env);
            }

            const char* opname = YogCode_get_op_name(op);
            TRACE("%p: PC=%u, op=%s", env, PC, opname);
            printf("%p: ---------------- end of stack ----------------\n", env);
            fflush(stdout);
        } while (0);
#endif

        PC += sizeof(uint8_t);
        switch (op) {
#include "src/eval.inc"
        default:
            YOG_ASSERT(env, FALSE, "Unknown instruction.");
            break;
        }
#undef POP_ARGS
#undef JUMP
#undef THREAD
#undef CONSTS
#undef POP
    }

    POP_JMPBUF(env);
#undef PUSH
#undef CODE
#undef PC

    POP_FRAME();

    RETURN(env, YUNDEF);
}

YogVal
YogEval_call_method_id(YogEnv* env, YogVal receiver, ID method, uint_t argc, YogVal* args)
{
    SAVE_ARG(env, receiver);
    YogVal attr = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, attr, retval);

    attr = YogVal_get_attr(env, receiver, method);
    YOG_ASSERT(env, IS_PTR(attr), "attribute isn't object (0x%08x)", attr);
    retval = YogCallable_call(env, attr, argc, args);

    RETURN(env, retval);
}

YogVal
YogEval_call_method_id2(YogEnv* env, YogVal receiver, ID method, uint_t argc, YogVal* args, YogVal blockarg)
{
    SAVE_ARGS2(env, receiver, blockarg);
    YogVal attr = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, attr, retval);

    attr = YogVal_get_attr(env, receiver, method);
    YOG_ASSERT(env, IS_PTR(attr), "Attribute isn't object.");
    retval = YogCallable_call2(env, attr, argc, args, blockarg);

    RETURN(env, retval);
}

void
YogEval_eval_package(YogEnv* env, YogVal pkg, YogVal code)
{
    SAVE_ARGS2(env, pkg, code);
    YogVal frame = YUNDEF;
    YogVal attrs = YUNDEF;
    PUSH_LOCALS2(env, frame, attrs);

    YogEval_push_finish_frame(env);

    frame = YogNameFrame_new(env);
    setup_script_frame(env, frame, code);
    PTR_AS(YogNameFrame, frame)->self = pkg;
    attrs = PTR_AS(YogObj, pkg)->attrs;
    PTR_AS(YogNameFrame, frame)->vars = attrs;
    SCRIPT_FRAME(frame)->globals = PTR_AS(YogNameFrame, frame)->vars;
    PUSH_FRAME(frame);

    YogEval_mainloop(env);

    RETURN_VOID(env);
}

YogVal
YogEval_eval_file(YogEnv* env, FILE* fp, const char* filename, const char* pkg_name)
{
    YOG_ASSERT(env, fp != NULL, "file pointer is NULL");

    SAVE_LOCALS(env);

    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    YogVal pkg = YUNDEF;
    PUSH_LOCALS3(env, stmts, code, pkg);

    stmts = YogParser_parse_file(env, fp, filename, FALSE);
    if (!IS_PTR(stmts)) {
        RETURN(env, YNIL);
    }
    code = YogCompiler_compile_package(env, filename, stmts);

    pkg = YogPackage_new(env);
    YogVM_register_package(env, env->vm, pkg_name, pkg);
    YogEval_eval_package(env, pkg, code);

    RETURN(env, pkg);
}

void
YogEval_push_finish_frame(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal frame = YUNDEF;
    PUSH_LOCAL(env, frame);

    frame = YogFinishFrame_new(env);
    setup_script_frame(env, frame, env->vm->finish_code);

    PUSH_FRAME(frame);

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
