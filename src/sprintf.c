#include "yog/config.h"
#include <alloca.h>
#include <stdarg.h>
#include "yog/class.h"
#include "yog/error.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

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
        pc++;
        switch (*pc) {
        case 'C':
        case 'S':
            n++;
            break;
        case '%':
        case 'I':
        case 'd':
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
        pc++;
        switch (*pc) {
        case 'C':
        case 'S':
            dest[n] = va_arg(aq, YogVal);
            n++;
            break;
        case '%':
            break;
        case 'I':
            va_arg(aq, ID);
            break;
        case 'd':
            va_arg(aq, int);
            break;
        case 's':
            va_arg(aq, char*);
            break;
        case 'u':
            va_arg(aq, unsigned int);
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
add_class_name(YogEnv* env, YogVal s, YogVal o)
{
    SAVE_ARGS2(env, s, o);
    YogVal klass = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS2(env, klass, name);

    klass = YogVal_get_class(env, o);
    ID id = PTR_AS(YogClass, klass)->name;
    name = YogVM_id2name(env, env->vm, id);
    YogString_add(env, s, name);

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
            YogString_add_char(env, s, *pc);
            continue;
        }
        pc++;
        switch (*pc) {
        case 'C':
            va_arg(ap, YogVal);
            add_class_name(env, s, pv[obj_index]);
            obj_index++;
            break;
        case 'I':
            name = YogVM_id2name(env, env->vm, va_arg(ap, ID));
            YogString_add(env, s, name);
            break;
        case 'S':
            va_arg(ap, YogVal);
            YogString_add(env, s, pv[obj_index]);
            obj_index++;
            break;
        case '%':
            YogString_add_char(env, s, *pc);
            break;
        case 'd':
            {
                int n = va_arg(ap, int);
                /* 64bit integer including '\0' needs at most 20 bytes */
                char buf[20];
                snprintf(buf, array_sizeof(buf), "%d", n);
                YogString_add_cstr(env, s, buf);
            }
            break;
        case 's':
            {
                char* p = va_arg(ap, char*);
                YogString_add_cstr(env, s, p);
            }
            break;
        case 'u':
            {
                unsigned int n = va_arg(ap, unsigned int);
                char buf[20];
                snprintf(buf, array_sizeof(buf), "%u", n);
                YogString_add_cstr(env, s, buf);
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
    YogVal* pv = (YogVal*)alloca(sizeof(YogVal) * n);
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
 * %I ID
 * %S Yog String object
 * %d integer
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
