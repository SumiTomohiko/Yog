#include "yog/config.h"
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "yog/binary.h"
#include "yog/callable.h"
#include "yog/code.h"
#include "yog/compile.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/frame.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/module.h"
#include "yog/package.h"
#include "yog/parser.h"
#include "yog/set.h"
#include "yog/string.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define DUMP_CODE(code)  YogCode_dump(env, code)
/* TODO: Remove this */
#define CUR_FRAME   env->frame

void
YogEval_pop_frame(YogEnv* env)
{
    env->frame = PTR_AS(YogFrame, env->frame)->prev;
}

void
YogEval_push_frame(YogEnv* env, YogVal frame)
{
    YogGC_UPDATE_PTR(env, PTR_AS(YogFrame, frame), prev, env->frame);
    env->frame = frame;
}

static YogVal
pop(YogEnv* env)
{
    YogVal frame = env->frame;
    uint_t size = PTR_AS(YogScriptFrame, frame)->stack_size;
    YOG_ASSERT(env, 0 < size, "Empty stack");
    YogVal val = PTR_AS(YogScriptFrame, frame)->locals_etc[size - 1];
    PTR_AS(YogScriptFrame, frame)->stack_size--;
    return val;
}

static YogVal
make_jmp_val(YogEnv* env, uint_t n)
{
    SAVE_LOCALS(env);
    YogVal objs = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, objs, val);

    if (n == 0) {
        objs = YogArray_of_size(env, 1);
        YogArray_push(env, objs, YNIL);
        RETURN(env, objs);
    }

    objs = YogArray_of_size(env, n);
    uint_t i;
    for (i = 0; i < n; i++) {
        val = pop(env);
        YogArray_push(env, objs, val);
    }

    RETURN(env, objs);
}

static void
set_lhs_composition(YogEnv* env, uint_t left, uint_t middle, uint_t right)
{
    PTR_AS(YogScriptFrame, CUR_FRAME)->lhs_left_num = left;
    PTR_AS(YogScriptFrame, CUR_FRAME)->lhs_middle_num = middle;
    PTR_AS(YogScriptFrame, CUR_FRAME)->lhs_right_num = right;
}

static void
return_middle(YogEnv* env, uint_t n)
{
    SAVE_LOCALS(env);
    YogVal prev = YUNDEF;
    YogVal val = YUNDEF;
    YogVal middle = YUNDEF;
    PUSH_LOCALS3(env, prev, val, middle);

    middle = YogArray_of_size(env, n);
    uint_t i;
    for (i = 0; i < n; i++) {
        val = pop(env);
        YogArray_push(env, middle, val);
    }

    prev = PTR_AS(YogFrame, CUR_FRAME)->prev;
    YogScriptFrame_push_stack(env, prev, middle);

    RETURN_VOID(env);
}

static void
move_stack_value(YogEnv* env, uint_t n)
{
    SAVE_LOCALS(env);
    YogVal prev = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, prev, val);

    prev = PTR_AS(YogFrame, CUR_FRAME)->prev;
    uint_t i;
    for (i = 0; i < n; i++) {
        val = pop(env);
        YogScriptFrame_push_stack(env, prev, val);
    }

    RETURN_VOID(env);
}

void
YogEval_push_returned_value(YogEnv* env, YogVal frame, YogVal val)
{
    uint_t left_num = PTR_AS(YogScriptFrame, frame)->lhs_left_num;
    uint_t middle_num = PTR_AS(YogScriptFrame, frame)->lhs_middle_num;
    uint_t right_num = PTR_AS(YogScriptFrame, frame)->lhs_right_num;
    if (left_num + middle_num + right_num == 0) {
        return;
    }
    if (0 < middle_num) {
        if (0 < left_num + right_num) {
            YogError_raise_ValueError(env, "too few multiple value");
        }
        YogHandle* h_frame = YogHandle_REGISTER(env, frame);
        YogHandle* h_val = YogHandle_REGISTER(env, val);
        YogVal middle = YogArray_of_size(env, 1);
        YogArray_push(env, middle, h_val->val);
        YogScriptFrame_push_stack(env, h_frame->val, middle);
    }
    else if (left_num != 1) {
        YogError_raise_ValueError(env, "number of multiple value unmatched");
        /* NOTREACHED */
    }
    else {
        YogScriptFrame_push_stack(env, frame, val);
    }
}

static void
move_returned_value(YogEnv* env, uint_t n)
{
    SAVE_LOCALS(env);
    YogVal prev_frame = YUNDEF;
    PUSH_LOCAL(env, prev_frame);

    prev_frame = PTR_AS(YogFrame, CUR_FRAME)->prev;
    if (n == 0) {
        YogEval_push_returned_value(env, prev_frame, YNIL);
        RETURN_VOID(env);
    }

    uint_t left_num = PTR_AS(YogScriptFrame, prev_frame)->lhs_left_num;
    uint_t middle_num = PTR_AS(YogScriptFrame, prev_frame)->lhs_middle_num;
    uint_t right_num = PTR_AS(YogScriptFrame, prev_frame)->lhs_right_num;
    if (left_num + middle_num + right_num == 0) {
        RETURN_VOID(env);
    }
    if (0 < middle_num) {
        if (n < left_num + right_num) {
            YogError_raise_ValueError(env, "too few multiple value");
        }
    }
    else if (left_num != n) {
        YogError_raise_ValueError(env, "number of multiple value unmatched");
        /* NOTREACHED */
    }
    move_stack_value(env, left_num);
    if (0 < middle_num) {
        return_middle(env, n - left_num - right_num);
    }
    move_stack_value(env, right_num);

    RETURN_VOID(env);
}

static void
push_from_array(YogEnv* env, YogVal vals, uint_t from, uint_t to)
{
    SAVE_ARG(env, vals);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    uint_t i;
    for (i = from; i < to; i++) {
        val = YogArray_at(env, vals, i);
        YogScriptFrame_push_stack(env, CUR_FRAME, val);
    }

    RETURN_VOID(env);
}

static void
push_array_from_array(YogEnv* env, YogVal vals, uint_t from, uint_t to)
{
    SAVE_ARG(env, vals);
    YogVal val = YUNDEF;
    YogVal a = YUNDEF;
    PUSH_LOCALS2(env, val, a);

    a = YogArray_of_size(env, to - from);
    uint_t i;
    for (i = from; i < to; i++) {
        val = YogArray_at(env, vals, i);
        YogArray_push(env, a, val);
    }
    YogScriptFrame_push_stack(env, CUR_FRAME, a);

    RETURN_VOID(env);
}

void
YogEval_push_returned_multi_value(YogEnv* env, YogVal vals)
{
    SAVE_ARG(env, vals);

    YOG_ASSERT(env, IS_PTR(vals), "jmp_val must be a pointer (0x%08x)", vals);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(vals) == TYPE_ARRAY, "jmp_val must be an array (0x%08x)", BASIC_OBJ_TYPE(vals));
    uint_t n = YogArray_size(env, vals);
    YOG_ASSERT(env, 0 < n, "jmp_val must contain more than one values (0x%08x)", n);

    uint_t left_num = PTR_AS(YogScriptFrame, CUR_FRAME)->lhs_left_num;
    uint_t middle_num = PTR_AS(YogScriptFrame, CUR_FRAME)->lhs_middle_num;
    uint_t right_num = PTR_AS(YogScriptFrame, CUR_FRAME)->lhs_right_num;
    if (left_num + middle_num + right_num == 0) {
        RETURN_VOID(env);
    }
    if (0 < middle_num) {
        if (n < left_num + right_num) {
            YogError_raise_ValueError(env, "too few multiple value");
            /* NOTREACHED */
        }
    }
    else if (left_num != n) {
        YogError_raise_ValueError(env, "number of multiple value unmatched. %u value(s) requested, but apply %u value(s)", left_num, n);
        /* NOTREACHED */
    }
    push_from_array(env, vals, 0, left_num);
    if (0 < middle_num) {
        push_array_from_array(env, vals, left_num, n - right_num);
    }
    push_from_array(env, vals, n - right_num, n);

    RETURN_VOID(env);
}

static void
push_jmp_val(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal vals = YUNDEF;
    PUSH_LOCAL(env, vals);

    vals = PTR_AS(YogThread, env->thread)->jmp_val;
    YogEval_push_returned_multi_value(env, vals);

    RETURN_VOID(env);
}

static void
exec_get_attr(YogEnv* env, YogVal obj, ID name)
{
    SAVE_ARG(env, obj);
    YogVal attr = YUNDEF;
    YogVal class_of_obj = YUNDEF;
    YogVal class_of_attr = YUNDEF;
    PUSH_LOCALS3(env, attr, class_of_obj, class_of_attr);

    class_of_obj = YogVal_get_class(env, obj);

    if (IS_PTR(obj) && ((PTR_AS(YogBasicObj, obj)->flags & HAS_ATTRS) != 0)) {
        attr = YogObj_get_attr(env, obj, name);
    }
    if (IS_UNDEF(attr)) {
        attr = YogClass_get_attr(env, class_of_obj, name);
    }
    if (IS_UNDEF(attr)) {
        YogError_raise_AttributeError(env, "%C object has no attribute \"%I\"", obj, name);
    }
    class_of_attr = YogVal_get_class(env, attr);
    void (*exec)(YogEnv*, YogVal, YogVal, YogVal) = PTR_AS(YogClass, class_of_attr)->exec_get_descr;
    if (exec == NULL) {
        YogScriptFrame_push_stack(env, env->frame, attr);
    }
    else {
        exec(env, attr, obj, class_of_obj);
    }

    RETURN_VOID(env);
}

static YogVal
get_outer_frame(YogEnv* env, uint_t level)
{
    SAVE_LOCALS(env);
    YogVal frame = YUNDEF;
    PUSH_LOCAL(env, frame);
    frame = SCRIPT_FRAME_OUTER_FRAMES(CUR_FRAME)[level - 1];
    RETURN(env, frame);
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

static void
setup_script_function(YogEnv* env, YogVal f, YogVal code)
{
    SAVE_ARGS2(env, f, code);
    YogVal frame = env->frame;
    PUSH_LOCAL(env, frame);

    YogGC_UPDATE_PTR(env, PTR_AS(YogFunction, f), code, code);
    YogGC_UPDATE_PTR(env, PTR_AS(YogFunction, f), globals, PTR_AS(YogScriptFrame, CUR_FRAME)->globals);
    YogGC_UPDATE_PTR(env, PTR_AS(YogFunction, f), outer_frame, frame);
    PTR_AS(YogScriptFrame, frame)->used_by_func = TRUE;

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
find_exception_table_entry(YogEnv* env, YogVal code, uint_t pc, uint_t* target)
{
    uint_t i;
    for (i = 0; i < PTR_AS(YogCode, code)->exc_tbl_size; i++) {
        YogVal exc_tbl = PTR_AS(YogCode, code)->exc_tbl;
        YogExceptionTableEntry* entry = &PTR_AS(YogExceptionTable, exc_tbl)->items[i];
        if ((entry->from < pc) && (pc <= entry->to)) {
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
    YogVal frame = env->frame;
    while (IS_PTR(frame = PTR_AS(YogFrame, frame)->prev)) {
        YogFrameType type = PTR_AS(YogFrame, frame)->type;
        switch (type) {
        case FRAME_SCRIPT:
            if (frame == target_frame) {
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
    YogVal frame = env->frame;
    while (IS_PTR(frame)) {
        const char* type;
        switch (PTR_AS(YogFrame, frame)->type) {
        case FRAME_C:       type = "FRAME_C";       break;
        case FRAME_CLASS:   type = "FRAME_CLASS";   break;
        case FRAME_FINISH:  type = "FRAME_FINISH";  break;
        case FRAME_METHOD:  type = "FRAME_METHOD";  break;
        case FRAME_PKG:     type = "FRAME_PKG";     break;
        default:            type = "unknown";       break;
        }
        TRACE("frame=0x%08x, type=%s", frame, type);
        frame = PTR_AS(YogFrame, frame)->prev;
    }
    TRACE("----------------");
}
#endif

void
YogEval_longjmp(YogEnv* env, int status)
{
    YogJmpBuf* buf = PTR_AS(YogThread, env->thread)->jmp_buf_list;
    YogHandleScope* scope = env->handles->scope;
    while (scope != buf->scope) {
        YogHandleScope_close(env);
        scope = scope->next;
    }

    env->handles->scope = buf->scope;
    env->locals->body = buf->locals;
    longjmp(buf->buf, status);
    /* NOTREACHED */
}

static void
skip_to_c_frame(YogEnv* env)
{
    YogVal frame = env->frame;
    while (IS_PTR(frame = PTR_AS(YogFrame, frame)->prev)) {
        YogFrameType type = PTR_AS(YogFrame, frame)->type;
        switch (type) {
        case FRAME_SCRIPT:
            break;
        case FRAME_C:
            env->frame = frame;
            return;
            break;
        case FRAME_FINISH:
            env->frame = PTR_AS(YogFrame, frame)->prev;
            return;
            break;
        default:
            YOG_BUG(env, "Invalid frame type (0x%x)", type);
            break;
        }
    }

    YOG_BUG(env, "Can't long jump current frame");
}

void
YogEval_longjmp_to_prev_buf(YogEnv* env, int status)
{
    POP_JMPBUF(env);
    YogEval_longjmp(env, status);
    /* NOTREACHED */
}

static void
long_jump(YogEnv* env, uint_t depth, int_t status, YogVal target_frame)
{
    YogHandle* h_target_frame = YogHandle_REGISTER(env, target_frame);

    YogHandle* objs = YogHandle_REGISTER(env, make_jmp_val(env, depth));
    detect_orphan(env, status, HDL2VAL(h_target_frame));

    YogVal thread = env->thread;
    YogGC_UPDATE_PTR(env, PTR_AS(YogThread, thread), jmp_val, HDL2VAL(objs));
    YogGC_UPDATE_PTR(env, PTR_AS(YogThread, thread), frame_to_long_jump, HDL2VAL(h_target_frame));

    YogEval_longjmp(env, status);
    /* NOTREACHED */
}

static YogVal
create_frame_for_names(YogEnv* env, YogVal code, YogVal vars)
{
    SAVE_ARGS2(env, code, vars);
    YogVal frame = YUNDEF;
    PUSH_LOCAL(env, frame);

    frame = YogFrame_get_script_frame(env, code, 1);
    uint_t pos = PTR_AS(YogScriptFrame, frame)->stack_capacity;
    YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), locals_etc[pos], vars);

    RETURN(env, frame);
}

static void
skip_c_frame(YogEnv* env)
{
    YogVal frame = env->frame;
    while (PTR_AS(YogFrame, frame)->type == FRAME_C) {
        frame = PTR_AS(YogFrame, frame)->prev;
    }
    env->frame = frame;
}

YogVal
YogEval_mainloop(YogEnv* env)
{
#define PC          (SCRIPT_FRAME(CUR_FRAME)->pc)
#undef CODE
#define CODE        PTR_AS(YogCode, SCRIPT_FRAME(CUR_FRAME)->code)
    YogHandleScope outer_scope;
    YogHandleScope_OPEN(env, &outer_scope);

    YogHandleScope inner_scope;
    YogJmpBuf jmpbuf;
    int_t status = setjmp(jmpbuf.buf);
    if (status == 0) {
        INIT_JMPBUF(env, jmpbuf);
        PUSH_JMPBUF(env->thread, jmpbuf);
    }
    else {
        skip_c_frame(env);

        switch (status) {
        case JMP_RAISE:
            {
                BOOL found = FALSE;
                if (PTR_AS(YogFrame, CUR_FRAME)->type != FRAME_C) {
                    uint_t target = 0;
                    found = find_exception_table_entry(env, (YogVal)CODE, PC, &target);
                    if (found) {
                        PC = target;
                    }
                }
                if (!found) {
                    skip_to_c_frame(env);
                    YogEval_longjmp_to_prev_buf(env, status);
                }
            }
            break;
        case JMP_RETURN:
        case JMP_BREAK:
            {
                YogVal frame = env->frame;
                while (IS_PTR(frame)) {
                    BOOL found = FALSE;
                    YogFrameType type = PTR_AS(YogFrame, frame)->type;
                    switch (type) {
                    case FRAME_C:
                        skip_to_c_frame(env);
                        YogEval_longjmp_to_prev_buf(env, status);
                        break;
                    case FRAME_SCRIPT:
                        {
                            YogVal thread = env->thread;
                            if (frame == PTR_AS(YogThread, thread)->frame_to_long_jump) {
                                CUR_FRAME = frame;
                                PC = SCRIPT_FRAME(CUR_FRAME)->pc;
                                push_jmp_val(env);
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
        YogHandleScope_OPEN(env, &inner_scope);
#define CONSTS(index)   (YogValArray_at(env, CODE->consts, index))
#define JUMP(m)         PC = m;
        OpCode op = (OpCode)PTR_AS(YogByteArray, CODE->insts)->items[PC];

#if 0
        do {
            uint_t lineno;
            if (!YogCode_get_lineno(env, PTR2VAL(CODE), PC, &lineno)) {
                lineno = 0;
            };
            const char* opname = YogCode_get_op_name(op);
            TRACE("%p: PC=%u, lineno=%u, op=%s", env, PC, lineno, opname);
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
#include "eval.inc"
        default:
            YOG_BUG(env, "Unknown instruction (0x%08x)", op);
            break;
        }
#undef JUMP
#undef CONSTS
        YogHandleScope_close(env);
    }
#undef CODE
#undef PC
    YOG_BUG(env, "Exited mainloop");
    /* NOTREACHED */
    return YUNDEF;
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
    retval = YogCallable_call_with_block(env, attr, argc, args, blockarg);

    RETURN(env, retval);
}

void
YogEval_eval_package(YogEnv* env, YogVal pkg, YogVal code)
{
    SAVE_ARGS2(env, pkg, code);
    YogVal frame = YUNDEF;
    YogVal attrs = PTR_AS(YogObj, pkg)->attrs;
    PUSH_LOCALS2(env, frame, attrs);

    YogEval_push_finish_frame(env);
    frame = create_frame_for_names(env, code, attrs);
    YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), globals, attrs);
    YogEval_push_frame(env, frame);
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
    code = YogCompiler_compile_package(env, filename, stmts);

    pkg = YogPackage_new(env);
    YogVM_register_package(env, env->vm, pkg_name, pkg);
    YogEval_eval_package(env, pkg, code);

    RETURN(env, pkg);
}

static YogVal
get_finish_frame(YogEnv* env)
{
    YogVal frame = YogThread_get_finish_frame(env, env->thread);
    if (IS_PTR(frame)) {
        return frame;
    }
    return YogFinishFrame_new(env);
}

void
YogEval_push_finish_frame(YogEnv* env)
{
    YogEval_push_frame(env, get_finish_frame(env));
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
