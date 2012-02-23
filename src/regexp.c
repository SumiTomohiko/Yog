#include "yog/config.h"
#include <ctype.h>
#include <string.h>
#include "corgi.h"
#include "yog/array.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/regexp.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_REGEXP(env, self)    do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_REGEXP)) { \
        YogError_raise_TypeError(env, "self must be Regexp"); \
    } \
} while (0)
#define CHECK_SELF_MATCH(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_MATCH)) { \
        YogError_raise_TypeError(env, "self must be Match"); \
    } \
} while (0)
#define CHECK_SELF_MATCH2(env, self)  do { \
    YogVal match = HDL2VAL((self)); \
    if (!IS_PTR(match) || (BASIC_OBJ_TYPE(match) != TYPE_MATCH)) { \
        YogError_raise_TypeError(env, "self must be Match"); \
    } \
} while (0)

#define ADD_ADDR(s, n) (YogChar*)((char*)(s) + (n))

static void
YogMatch_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogMatch* match = PTR_AS(YogMatch, ptr);
#define KEEP(member)    YogGC_KEEP(env, match, member, keeper, heap)
    KEEP(str);
    KEEP(regexp);
#undef KEEP
}

static void
YogMatch_finalize(YogEnv* env, void* ptr)
{
    YogMatch* match = PTR_AS(YogMatch, ptr);
    corgi_fini_match(&match->corgi_match);
}

YogVal
YogMatch_new(YogEnv* env, YogHandle* str, YogHandle* regexp)
{
    YogVal match = ALLOC_OBJ(env, YogMatch_keep_children, YogMatch_finalize, YogMatch);
    YogBasicObj_init(env, match, TYPE_MATCH, 0, env->vm->cMatch);
    YogGC_UPDATE_PTR(env, PTR_AS(YogMatch, match), str, HDL2VAL(str));
    YogGC_UPDATE_PTR(env, PTR_AS(YogMatch, match), regexp, HDL2VAL(regexp));
    corgi_init_match(&PTR_AS(YogMatch, match)->corgi_match);

    return match;
}

static void
YogRegexp_finalize(YogEnv* env, void* ptr)
{
    YogRegexp* regexp = PTR_AS(YogRegexp, ptr);
    corgi_fini_regexp(&regexp->corgi_regexp);
}

YogVal
YogRegexp_new(YogEnv* env, YogVal pattern, BOOL ignore_case)
{
    YogHandle* h = VAL2HDL(env, pattern);
    YogVal regexp = ALLOC_OBJ(env, YogBasicObj_keep_children, YogRegexp_finalize, YogRegexp);
    YogBasicObj_init(env, regexp, TYPE_REGEXP, 0, env->vm->cRegexp);
    corgi_init_regexp(&PTR_AS(YogRegexp, regexp)->corgi_regexp);

    CorgiChar* begin = STRING_CHARS(HDL2VAL(h));
    CorgiOptions opts = 0;
    if (ignore_case) {
        opts |= CORGI_OPT_IGNORE_CASE;
    }
    CorgiRegexp* reg = &PTR_AS(YogRegexp, regexp)->corgi_regexp;
    CorgiChar* end = begin + STRING_SIZE(HDL2VAL(h));
    CorgiStatus status = corgi_compile(reg, begin, end, opts);
    if (status != CORGI_OK) {
        const char* msg = corgi_strerror(status);
        YogError_raise_ValueError(env, "corgi error: %s", msg);
        /* NOTREACHED */
    }

    return regexp;
}

static int_t
group_name2id(YogEnv* env, YogVal self, YogVal group)
{
    YOG_ASSERT(env, IS_PTR(group), "invalid group (0x%x)", group);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(group) == TYPE_STRING, "invalid group type (0x%x)", BASIC_OBJ_TYPE(group));

    YogVal regexp = PTR_AS(YogMatch, self)->regexp;
    CorgiRegexp* corgi_regexp = &PTR_AS(YogRegexp, regexp)->corgi_regexp;
    YogChar* begin = STRING_CHARS(group);
    YogChar* end = begin + STRING_SIZE(group);
    uint_t id;
    CorgiStatus status = corgi_group_name2id(corgi_regexp, begin, end, &id);
    if (status != CORGI_OK) {
        YogError_raise_IndexError(env, "No such group: %S", group);
    }

    return id + 1;
}

static void
raise_invalid_group(YogEnv* env, YogVal group)
{
    SAVE_ARG(env, group);
    YogError_raise_TypeError(env, "group must be a Fixnum, String or nil, not %C", group);
    /* NOTREACHED */
    RETURN_VOID(env);
}

static void
get_group_range(YogEnv* env, YogVal self, int_t id, int_t* begin, int_t* end)
{
    CorgiMatch* match = &PTR_AS(YogMatch, self)->corgi_match;
    if (id == 0) {
        *begin = match->begin;
        *end = match->end;
        return;
    }
    if (corgi_get_group_range(match, id - 1, begin, end) == CORGI_OK) {
        return;
    }
    YogError_raise_IndexError(env, "No such group: %d", id);
}

static YogVal
group_num(YogEnv* env, YogHandle* self, int_t group)
{
    int_t begin;
    int_t end;
    get_group_range(env, HDL2VAL(self), group, &begin, &end);
    if (begin < 0) {
        return YNIL;
    }
    uint_t size = end - begin;
    YogVal retval = YogString_of_size(env, size);
    YogVal s = HDL_AS(YogMatch, self)->str;
    uint_t bytes_num = sizeof(YogChar) * size;
    memcpy(STRING_CHARS(retval), STRING_CHARS(s) + begin, bytes_num);
    STRING_SIZE(retval) = size;
    return retval;
}

static YogVal
group_str(YogEnv* env, YogHandle* self, YogHandle* group)
{
    return group_num(env, self, group_name2id(env, HDL2VAL(self), HDL2VAL(group)));
}

static YogVal
group(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* group)
{
    CHECK_SELF_MATCH2(env, self);

    if ((group == NULL) || IS_NIL(HDL2VAL(group))) {
        return group_num(env, self, 0);
    }
    if (IS_FIXNUM(HDL2VAL(group))) {
        return group_num(env, self, VAL2INT(HDL2VAL(group)));
    }
    if (IS_PTR(HDL2VAL(group)) && (BASIC_OBJ_TYPE(HDL2VAL(group)) == TYPE_STRING)) {
        return group_str(env, self, group);
    }
    raise_invalid_group(env, HDL2VAL(group));
    return YUNDEF;
}

static YogVal
start_num(YogEnv* env, YogVal self, int_t group)
{
    int_t begin;
    int_t end;
    get_group_range(env, self, group, &begin, &end);
    if (begin < 0) {
        return YNIL;
    }
    return YogVal_from_int(env, begin);
}

static YogVal
start_str(YogEnv* env, YogVal self, YogVal group)
{
    YOG_ASSERT(env, IS_PTR(group), "invalid group (0x%x)", group);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(group) == TYPE_STRING, "invalid group type (0x%0x)", BASIC_OBJ_TYPE(group));
    return start_num(env, self, group_name2id(env, self, group));
}

static YogVal
start(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal group = YNIL;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, group, retval);

    YogCArg params[] = { { "|", NULL }, { "group", &group }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "start", params, args, kw);
    CHECK_SELF_MATCH(env, self);

    if (IS_FIXNUM(group)) {
        retval = start_num(env, self, VAL2INT(group));
    }
    else if (IS_NIL(group)) {
        retval = start_num(env, self, 0);
    }
    else if (IS_PTR(group) && (BASIC_OBJ_TYPE(group) == TYPE_STRING)) {
        retval = start_str(env, self, group);
    }
    else {
        raise_invalid_group(env, group);
    }

    RETURN(env, retval);
}

static YogVal
end_num(YogEnv* env, YogVal self, int_t group)
{
    int_t begin;
    int_t end;
    get_group_range(env, self, group, &begin, &end);
    if (begin < 0) {
        return YNIL;
    }
    return YogVal_from_int(env, end);
}

static YogVal
end_str(YogEnv* env, YogVal self, YogVal group)
{
    YOG_ASSERT(env, IS_PTR(group), "invalid group (0x%x)", group);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(group) == TYPE_STRING, "invalid group type (0x%0x)", BASIC_OBJ_TYPE(group));
    return end_num(env, self, group_name2id(env, self, group));
}

YogVal
YogRegexp_binop_search(YogEnv* env, YogHandle* self, YogHandle* s)
{
    if (!IS_PTR(HDL2VAL(s)) || (BASIC_OBJ_TYPE(HDL2VAL(s)) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "Can't convert %C object to String implicitly", HDL2VAL(s));
        /* NOTREACHED */
    }
    return YogString_search(env, s, self, 0);
}

static YogVal
do_match_or_search(YogEnv* env, YogHandle* self, YogHandle* s, YogHandle* pos, YogVal (*f)(YogEnv*, YogHandle*, YogHandle*, int_t))
{
    YogMisc_check_String(env, s, "s");
    YogMisc_check_Fixnum_optional(env, pos, "pos");
    return f(env, s, self, pos == NULL ? 0 : VAL2INT(HDL2VAL(pos)));
}

static YogVal
match(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* s, YogHandle* pos)
{
    return do_match_or_search(env, self, s, pos, YogString_match);
}

static YogVal
search(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* s, YogHandle* pos)
{
    return do_match_or_search(env, self, s, pos, YogString_search);
}

static YogVal
end(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal group = YNIL;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, group, retval);

    YogCArg params[] = { { "|", NULL }, { "group", &group }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "end", params, args, kw);
    CHECK_SELF_MATCH(env, self);

    if (IS_FIXNUM(group)) {
        retval = end_num(env, self, VAL2INT(group));
    }
    else if (IS_NIL(group)) {
        retval = end_num(env, self, 0);
    }
    else if (IS_PTR(group) && (BASIC_OBJ_TYPE(group) == TYPE_STRING)) {
        retval = end_str(env, self, group);
    }
    else {
        raise_invalid_group(env, group);
    }

    RETURN(env, retval);
}

void
YogRegexp_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cRegexp = YUNDEF;
    YogVal cMatch = YUNDEF;
    PUSH_LOCALS2(env, cRegexp, cMatch);
    YogVM* vm = env->vm;

    cRegexp = YogClass_new(env, "Regexp", vm->cObject);
#define DEFINE_METHOD(name, ...) do { \
    YogClass_define_method2(env, cRegexp, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD("match", match, "s", "|", "pos", NULL);
    DEFINE_METHOD("search", search, "s", "|", "pos", NULL);
#undef DEFINE_METHOD
    vm->cRegexp = cRegexp;

    cMatch = YogClass_new(env, "Match", vm->cObject);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cMatch, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("start", start);
    DEFINE_METHOD("end", end);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...) do { \
    YogClass_define_method2(env, cMatch, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("group", group, "|", "group", NULL);
#undef DEFINE_METHOD2
    vm->cMatch = cMatch;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
