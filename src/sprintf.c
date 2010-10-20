#include "yog/config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include "yog/bignum.h"
#include "yog/class.h"
#include "yog/compat.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

static const char*
read_flags(const char* s, uint_t* l)
{
    *l = 0;

    const char* pc = s;
    while (1) {
        switch (*pc) {
        case 'l':
            (*l)++;
            break;
        default:
            return pc;
            break;
        }
        pc++;
    }
}

static uint_t
count_objects(YogEnv* env, const char* fmt)
{
    SAVE_LOCALS(env);
    uint_t n = 0;
    const char* pc;
    for (pc = fmt; *pc != '\0'; pc++) {
        if (*pc != '%') {
            continue;
        }
        uint_t l;
        pc = read_flags(pc + 1, &l);
        switch (*pc) {
        case 'C':
        case 'D':
        case 'S':
            n++;
            break;
        case '%':
        case 'I':
        case 'c':
        case 'd':
        case 'p':
        case 's':
        case 'u':
            break;
        default:
            YOG_BUG(env, "invalid format charactor '%c'", *pc);
            break;
        }
    }

    RETURN(env, n);
}

static void
store_objects(YogEnv* env, YogVal* dest, const char* fmt, va_list ap)
{
    SAVE_LOCALS(env);
    va_list aq;
    va_copy(aq, ap);

    uint_t n = 0;
    const char* pc;
    for (pc = fmt; *pc != '\0'; pc++) {
        if (*pc != '%') {
            continue;
        }
        uint_t l;
        pc = read_flags(pc + 1, &l);
        switch (*pc) {
        case 'C':
        case 'D':
        case 'S':
            dest[n] = va_arg(aq, YogVal);
            n++;
            break;
        case '%':
            break;
        case 'I':
            va_arg(aq, ID);
            break;
        case 'c':
            va_arg(aq, int);
            break;
        case 'd':
            if (l == 0) {
                va_arg(aq, int);
            }
            else {
                va_arg(aq, long long);
            }
            break;
        case 'p':
            va_arg(aq, void*);
            break;
        case 's':
            va_arg(aq, char*);
            break;
        case 'u':
            switch (l) {
            case 0:
                va_arg(aq, unsigned int);
                break;
            case 1:
                va_arg(aq, unsigned long);
                break;
            default:
                va_arg(aq, unsigned long long);
                break;
            }
            break;
        default:
            YOG_BUG(env, "invalid format charactor '%c'", *pc);
            break;
        }
    }

    va_end(aq);
    RETURN_VOID(env);
}

static void
add_num(YogEnv* env, YogVal s, YogVal o)
{
    SAVE_ARGS2(env, s, o);
    YogVal t = YUNDEF;
    PUSH_LOCAL(env, t);

    if (IS_FIXNUM(o)) {
        char buf[21]; /* 64bit integer with '\0' needs at most 21 bytes */
        snprintf(buf, array_sizeof(buf), "%d", VAL2INT(o));
        YogString_append_string(env, s, buf);
    }
    else if (IS_PTR(o) && (BASIC_OBJ_TYPE(o) == TYPE_BIGNUM)) {
        t = YogBignum_to_s(env, o);
        YogString_append(env, s, t);
    }
    else {
        YogError_raise_TypeError(env, "Object for %%D must be Fixnum or Bignum, not %C", o);
    }

    RETURN_VOID(env);
}

static void
add_class_name(YogEnv* env, YogVal s, YogVal o)
{
    SAVE_ARGS2(env, s, o);
    YogVal klass = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS2(env, klass, name);

    klass = YogVal_get_class(env, o);
    ID id = PTR_AS(YogClass, klass)->name;
    name = YogVM_id2name(env, env->vm, id);
    YogString_append(env, s, name);

    RETURN_VOID(env);
}

static YogVal
conv_to_string(YogEnv* env, YogVal o)
{
    SAVE_ARG(env, o);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

#define IS_STRING(o)    (IS_PTR(o) && (BASIC_OBJ_TYPE(o) == TYPE_STRING))
    if (IS_STRING(o)) {
        RETURN(env, o);
    }

    s = YogEval_call_method0(env, o, "to_s");
    if (!IS_STRING(s)) {
        YogError_raise_TypeError(env, "to_s returned non-string");
    }
#undef IS_STRING

    RETURN(env, s);
}

static void
add_string(YogEnv* env, YogVal s, YogVal o)
{
    SAVE_ARGS2(env, s, o);
    YogVal t = YUNDEF;
    PUSH_LOCAL(env, t);

    t = conv_to_string(env, o);
    YogString_append(env, s, t);

    RETURN_VOID(env);
}

static YogVal
format(YogEnv* env, const char* fmt, va_list ap, YogVal* pv)
{
    SAVE_LOCALS(env);
    YogVal s = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS2(env, s, name);
    s = YogString_new(env);

    uint_t obj_index = 0;
    const char* pc;
    for (pc = fmt; *pc != '\0'; pc++) {
        if (*pc != '%') {
            YogString_push(env, s, *pc);
            continue;
        }
        uint_t l;
        pc = read_flags(pc + 1, &l);
        switch (*pc) {
        case 'C':
            va_arg(ap, YogVal);
            add_class_name(env, s, pv[obj_index]);
            obj_index++;
            break;
        case 'D':
            va_arg(ap, YogVal);
            add_num(env, s, pv[obj_index]);
            obj_index++;
            break;
        case 'I':
            name = YogVM_id2name(env, env->vm, va_arg(ap, ID));
            YogString_append(env, s, name);
            break;
        case 'S':
            va_arg(ap, YogVal);
            add_string(env, s, pv[obj_index]);
            obj_index++;
            break;
        case '%':
            YogString_push(env, s, *pc);
            break;
        case 'c':
            {
                char buf[2];
                snprintf(buf, array_sizeof(buf), "%c", va_arg(ap, int));
                YogString_append_string(env, s, buf);
            }
            break;
        case 'd':
            {
                /* 64bit integer including '\0' needs at most 21 bytes */
                char buf[21];
                if (l == 0) {
                    int n = va_arg(ap, int);
                    YogSysdeps_snprintf(buf, array_sizeof(buf), "%d", n);
                }
                else {
                    long long n = va_arg(ap, long long);
                    YogSysdeps_snprintf(buf, array_sizeof(buf), "%lld", n);
                }
                YogString_append_string(env, s, buf);
            }
            break;
        case 'p':
            {
                /* 2 at head is for "0x", 1 at tail is for "\0" */
                uint_t size = 2 + 2 * sizeof(void*) + 1;
                char buf[size];
                snprintf(buf, size, "%p", va_arg(ap, void*));
                YogString_append_string(env, s, buf);
            }
            break;
        case 's':
            {
                char* p = va_arg(ap, char*);
                YogString_append_string(env, s, p);
            }
            break;
        case 'u':
            {
                char buf[21];
                unsigned int u;
                unsigned long lu;
                unsigned long long llu;
                switch (l) {
                case 0:
                    u = va_arg(ap, unsigned int);
                    YogSysdeps_snprintf(buf, array_sizeof(buf), "%u", u);
                    break;
                case 1:
                    lu = va_arg(ap, unsigned long);
                    YogSysdeps_snprintf(buf, array_sizeof(buf), "%lu", lu);
                    break;
                default:
                    llu = va_arg(ap, unsigned long long);
                    YogSysdeps_snprintf(buf, array_sizeof(buf), "%llu", llu);
                    break;
                }
                YogString_append_string(env, s, buf);
            }
            break;
        default:
            YOG_BUG(env, "invalid format charactor '%c'", *pc);
            break;
        }
    }

    RETURN(env, s);
}

YogVal
YogSprintf_vsprintf(YogEnv* env, const char* fmt, va_list ap)
{
    SAVE_LOCALS(env);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    uint_t n = count_objects(env, fmt);
    YogVal* pv = (YogVal*)YogSysdeps_alloca(sizeof(YogVal) * n);
    uint_t i;
    for (i = 0; i < n; i++) {
        pv[i] = YUNDEF;
    }
    PUSH_LOCALSX(env, n, pv);
    store_objects(env, pv, fmt, ap);

    s = format(env, fmt, ap, pv);

    RETURN(env, s);
}

/**
 * fmt: ascii string. This function can't handle multibyte strings.
 * %C Yog object (class's name)
 * %D Fixnum or Bignum
 * %I ID
 * %S Yog String object
 * %c char
 * %d integer
 * %lld long long
 * %llu unsigned long long
 * %p pointer
 * %s C string
 * %u unsigned integer
 */
YogVal
YogSprintf_sprintf(YogEnv* env, const char* fmt, ...)
{
    SAVE_LOCALS(env);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    va_list ap;
    va_start(ap, fmt);
    s = YogSprintf_vsprintf(env, fmt, ap);
    va_end(ap);

    RETURN(env, s);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
