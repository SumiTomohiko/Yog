#include <string.h>
#include "oniguruma.h"
#include "yog/array.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/get_args.h"
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
    onig_region_free(match->onig_region, 1);
    match->onig_region = NULL;
}

YogVal
YogMatch_new(YogEnv* env, YogVal str, YogVal regexp, OnigRegion* onig_region)
{
    SAVE_ARGS2(env, str, regexp);

    YogVal match = ALLOC_OBJ(env, YogMatch_keep_children, YogMatch_finalize, YogMatch);
    YogBasicObj_init(env, match, TYPE_MATCH, 0, env->vm->cMatch);
    YogGC_UPDATE_PTR(env, PTR_AS(YogMatch, match), str, str);
    YogGC_UPDATE_PTR(env, PTR_AS(YogMatch, match), regexp, regexp);
    PTR_AS(YogMatch, match)->onig_region = onig_region;

    RETURN(env, match);
}

static void
YogRegexp_finalize(YogEnv* env, void* ptr)
{
    YogRegexp* regexp = PTR_AS(YogRegexp, ptr);
    onig_free(regexp->onig_regexp);
    regexp->onig_regexp = NULL;
}

YogVal
YogRegexp_new(YogEnv* env, YogVal pattern, OnigOptionType option)
{
    OnigRegex onig_regexp = NULL;
    YogVal body = PTR_AS(YogString, pattern)->body;
    OnigUChar* pattern_begin = (OnigUChar*)PTR_AS(YogCharArray, body)->items;
    uint_t size = STRING_SIZE(pattern);
    OnigUChar* pattern_end = pattern_begin + size - 1;
    OnigSyntaxType* syntax = ONIG_SYNTAX_DEFAULT;
    OnigErrorInfo einfo;

    YogVal enc = PTR_AS(YogString, pattern)->encoding;
    OnigEncoding onig = PTR_AS(YogEncoding, enc)->onig_enc;
    int_t r = onig_new(&onig_regexp, pattern_begin, pattern_end, option, onig, syntax, &einfo);
    if (r != ONIG_NORMAL) {
        return YNIL;
    }

    YogVal regexp = ALLOC_OBJ(env, YogBasicObj_keep_children, YogRegexp_finalize, YogRegexp);
    YogBasicObj_init(env, regexp, TYPE_REGEXP, 0, env->vm->cRegexp);
    PTR_AS(YogRegexp, regexp)->onig_regexp = onig_regexp;

    return regexp;
}

static int_t
group2indexes(YogEnv* env, YogVal self, YogVal group, int_t** num_list)
{
    YOG_ASSERT(env, IS_PTR(group), "invalid group (0x%x)", group);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(group) == TYPE_STRING, "invalid group type (0x%x)", BASIC_OBJ_TYPE(group));
    YOG_ASSERT(env, num_list != NULL, "num_list is NULL");
    SAVE_ARG(env, group);
    YogVal regexp = YUNDEF;
    PUSH_LOCAL(env, regexp);

    regexp = PTR_AS(YogMatch, self)->regexp;
    OnigRegex onig_regexp = PTR_AS(YogRegexp, regexp)->onig_regexp;
    OnigUChar* name_begin = (OnigUChar*)STRING_CSTR(group);
    OnigUChar* name_end = name_begin + STRING_SIZE(group) - 1;
    int_t r = onig_name_to_group_numbers(onig_regexp, name_begin, name_end, num_list);
    if (r < 1) {
        YogError_raise_IndexError(env, "no such group");
    }

    RETURN(env, r);
}

static void
raise_invalid_group(YogEnv* env, YogVal group)
{
    SAVE_ARG(env, group);
    YogError_raise_TypeError(env, "group must be a Fixnum, String or nil, not %C", group);
    /* NOTREACHED */
    RETURN_VOID(env);
}

static YogVal
group_num(YogEnv* env, YogVal self, int_t group)
{
    SAVE_ARG(env, self);
    YogVal s = YUNDEF;
    YogVal str = YUNDEF;
    YogVal to_body = YUNDEF;
    YogVal from_body = YUNDEF;
    PUSH_LOCALS4(env, s, str, to_body, from_body);

    OnigRegion* region = PTR_AS(YogMatch, self)->onig_region;
    if ((group < 0) || (region->num_regs <= group)) {
        YogError_raise_IndexError(env, "no such group");
    }
    int_t begin = region->beg[group];
    int_t end = region->end[group];
    int_t size = end - begin;
    s = YogString_of_size(env, size + 1);
    str = PTR_AS(YogMatch, self)->str;
    memcpy(STRING_CSTR(s), &STRING_CSTR(str)[begin], size);
    STRING_CSTR(s)[size] = '\0';
    STRING_SIZE(s) = size + 1;
    STRING_ENCODING(s) = STRING_ENCODING(str);

    RETURN(env, s);
}

static YogVal
group_str(YogEnv* env, YogVal self, YogVal group)
{
    YOG_ASSERT(env, IS_PTR(group), "invalid group (0x%x)", group);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(group) == TYPE_STRING, "invalid group type (0x%x)", group);
    SAVE_ARGS2(env, self, group);
    YogVal s = YUNDEF;
    YogVal a = YUNDEF;
    PUSH_LOCALS2(env, s, a);

    int_t* num_list;
    int_t num = group2indexes(env, self, group, &num_list);
    if (num == 1) {
        s = group_num(env, self, num_list[0]);
        RETURN(env, s);
    }

    YOG_ASSERT(env, 1 < num, "invalid num (0x%x)", num);
    a = YogArray_of_size(env, num);
    uint_t i;
    for (i = 0; i < num; i++) {
        s = group_num(env, self, num_list[i]);
        YogArray_push(env, a, s);
    }

    RETURN(env, a);
}

static YogVal
group(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal group = YNIL;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, group, retval);

    YogCArg params[] = { { "|", NULL }, { "group", &group }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "group", params, args, kw);
    CHECK_SELF_MATCH(env, self);

    if (IS_FIXNUM(group)) {
        retval = group_num(env, self, VAL2INT(group));
    }
    else if (IS_NIL(group)) {
        retval = group_num(env, self, 0);
    }
    else if (IS_PTR(group) && (BASIC_OBJ_TYPE(group) == TYPE_STRING)) {
        retval = group_str(env, self, group);
    }
    else {
        raise_invalid_group(env, group);
    }

    RETURN(env, retval);
}

static int_t
ptr2index(YogEnv* env, YogVal s, const char* ptr)
{
    SAVE_ARG(env, s);
    YogVal enc = YUNDEF;
    PUSH_LOCAL(env, enc);

    uint_t index = 0;
    enc = PTR_AS(YogString, s)->encoding;
    const char* p = STRING_CSTR(s);
    while (p < ptr) {
        p += YogEncoding_mbc_size(env, enc, p);
        index++;
    }

    RETURN(env, index);
}

static YogVal
start_num(YogEnv* env, YogVal self, int_t group)
{
    SAVE_ARG(env, self);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    OnigRegion* region = PTR_AS(YogMatch, self)->onig_region;
    if ((group < 0) || (region->num_regs <= group)) {
        YogError_raise_IndexError(env, "no such group");
    }
    s = PTR_AS(YogMatch, self)->str;
    const char* start = STRING_CSTR(s) + region->beg[group];
    int_t n = ptr2index(env, s, start);

    RETURN(env, INT2VAL(n));
}

static YogVal
start_str(YogEnv* env, YogVal self, YogVal group)
{
    YOG_ASSERT(env, IS_PTR(group), "invalid group (0x%x)", group);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(group) == TYPE_STRING, "invalid group type (0x%0x)", BASIC_OBJ_TYPE(group));
    SAVE_ARGS2(env, self, group);
    YogVal n = YUNDEF;
    YogVal a = YUNDEF;
    PUSH_LOCALS2(env, n, a);

    int_t* num_list;
    int_t num = group2indexes(env, self, group, &num_list);
    if (num == 1) {
        n = start_num(env, self, num_list[0]);
        RETURN(env, n);
    }

    YOG_ASSERT(env, 1 < num, "invalid num (0x%x)", num);
    a = YogArray_of_size(env, num);
    uint_t i;
    for (i = 0; i < num; i++) {
        n = start_num(env, self, num_list[i]);
        YogArray_push(env, a, n);
    }

    RETURN(env, a);
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
    SAVE_ARG(env, self);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    OnigRegion* region = PTR_AS(YogMatch, self)->onig_region;
    if ((group < 0) || (region->num_regs <= group)) {
        YogError_raise_IndexError(env, "no such group");
    }
    s = PTR_AS(YogMatch, self)->str;
    const char* end = STRING_CSTR(s) + region->end[group];
    int_t n = ptr2index(env, s, end);

    RETURN(env, INT2VAL(n));
}

static YogVal
end_str(YogEnv* env, YogVal self, YogVal group)
{
    YOG_ASSERT(env, IS_PTR(group), "invalid group (0x%x)", group);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(group) == TYPE_STRING, "invalid group type (0x%0x)", BASIC_OBJ_TYPE(group));
    SAVE_ARGS2(env, self, group);
    YogVal n = YUNDEF;
    YogVal a = YUNDEF;
    PUSH_LOCALS2(env, n, a);

    int_t* num_list;
    int_t num = group2indexes(env, self, group, &num_list);
    if (num == 1) {
        n = end_num(env, self, num_list[0]);
        RETURN(env, n);
    }

    YOG_ASSERT(env, 1 < num, "invalid num (0x%x)", num);
    a = YogArray_of_size(env, num);
    uint_t i;
    for (i = 0; i < num; i++) {
        n = end_num(env, self, num_list[i]);
        YogArray_push(env, a, n);
    }

    RETURN(env, a);
}

static YogVal
match(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal m = YUNDEF;
    YogVal pos = YUNDEF;
    YogVal s = YUNDEF;
    PUSH_LOCALS3(env, m, pos, s);
    CHECK_SELF_REGEXP(env, self);
    YogCArg params[] = {
        { "s", &s }, { "|", NULL }, { "pos", &pos }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "match", params, args, kw);
    if (!IS_PTR(s) || (BASIC_OBJ_TYPE(s) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "s must be String");
    }
    if (!IS_UNDEF(pos) && !IS_FIXNUM(pos)) {
        YogError_raise_TypeError(env, "pos must be Fixnum");
    }

    m = YogString_match(env, s, self, IS_UNDEF(pos) ? 0 : VAL2INT(pos));

    RETURN(env, m);
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
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cRegexp, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("match", match);
#undef DEFINE_METHOD
    vm->cRegexp = cRegexp;

    cMatch = YogClass_new(env, "Match", vm->cObject);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cMatch, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("group", group);
    DEFINE_METHOD("start", start);
    DEFINE_METHOD("end", end);
#undef DEFINE_METHOD
    vm->cMatch = cMatch;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
