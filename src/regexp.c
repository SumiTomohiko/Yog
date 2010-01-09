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
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_MATCH)) { \
        YogError_raise_TypeError(env, "self must be Match"); \
    } \
} while (0)

static void
YogMatch_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogMatch* match = PTR_AS(YogMatch, ptr);
#define KEEP(member)    YogGC_keep(env, &match->member, keeper, heap)
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
    PTR_AS(YogMatch, match)->str = str;
    PTR_AS(YogMatch, match)->regexp = regexp;
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
group2index(YogEnv* env, YogMatch* match, YogVal arg)
{
    SAVE_ARG(env, arg);
    YogVal klass = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS2(env, klass, name);

    if (IS_PTR(arg) && (BASIC_OBJ_TYPE(arg) == TYPE_STRING)) {
        YogString* s = PTR_AS(YogString, arg);
        OnigRegex onig_regexp = PTR_AS(YogRegexp, match->regexp)->onig_regexp;
        YogVal body = s->body;
        OnigUChar* name_begin = (OnigUChar*)PTR_AS(YogCharArray, body)->items;
        OnigUChar* name_end = name_begin + STRING_SIZE(arg) - 1;
        int_t* num_list = NULL;
        int_t r = onig_name_to_group_numbers(onig_regexp, name_begin, name_end, &num_list);
        YOG_ASSERT(env, r == 1, "TODO: index error?");
        RETURN(env, num_list[0]);
    }
    else if (IS_FIXNUM(arg)) {
        RETURN(env, VAL2INT(arg));
    }
    else if (IS_NIL(arg)) {
        RETURN(env, 0);
    }

    klass = YogVal_get_class(env, arg);
    name = YogVM_id2name(env, env->vm, PTR_AS(YogClass, klass)->name);
    YogError_raise_TypeError(env, "group must be a Fixnum, String or nil, not %s", STRING_CSTR(name));

    /* NOTREACHED */
    RETURN(env, SIGNED_MIN);
}

static YogVal
group(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal arg0 = YNIL;
    PUSH_LOCAL(env, arg0);

    YogCArg params[] = { { "|", NULL }, { "group", &arg0 }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "group", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    YogMatch* match = PTR_AS(YogMatch, self);
    int_t index = group2index(env, match, arg0);

    OnigRegion* region = match->onig_region;
    if ((index < 0) || (region->num_regs <= index)) {
        YogError_raise_IndexError(env, "no such group");
    }
    int_t begin = region->beg[index];
    int_t end = region->end[index];
    int_t size = end - begin;
    YogVal s = YogString_from_size(env, size + 1);
    YogVal str = PTR_AS(YogMatch, self)->str;
    YogVal to_body = PTR_AS(YogString, s)->body;
    char* p = PTR_AS(YogCharArray, to_body)->items;
    YogVal from_body = PTR_AS(YogString, str)->body;
    const char* q = &PTR_AS(YogCharArray, from_body)->items[begin];
    memcpy(p, q, size);
    PTR_AS(YogCharArray, to_body)->items[size] = '\0';

    RETURN(env, s);
}

static int_t
ptr2index(YogEnv* env, YogString* s, const char* ptr)
{
    uint_t index = 0;
    YogVal enc = s->encoding;
    YogVal body = s->body;
    const char* p = PTR_AS(YogCharArray, body)->items;
    while (p < ptr) {
        p += YogEncoding_mbc_size(env, enc, p);
        index++;
    }

    return index;
}

static YogVal
start(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal arg0 = YNIL;
    PUSH_LOCAL(env, arg0);

    YogCArg params[] = { { "|", NULL }, { "group", &arg0 }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "start", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    YogMatch* match = PTR_AS(YogMatch, self);
    int_t index = group2index(env, match, arg0);
    OnigRegion* region = match->onig_region;
    if ((index < 0) || (region->num_regs <= index)) {
        YogError_raise_IndexError(env, "no such group");
    }
    YogString* s = PTR_AS(YogString, match->str);
    YogVal body = s->body;
    const char* start = PTR_AS(YogCharArray, body)->items + region->beg[index];
    int_t n = ptr2index(env, s, start);

    RETURN(env, INT2VAL(n));
}

static YogVal
end(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal arg0 = YNIL;
    PUSH_LOCAL(env, arg0);

    YogCArg params[] = { { "|", NULL }, { "group", &arg0 }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "end", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    YogMatch* match = PTR_AS(YogMatch, self);
    int_t index = group2index(env, match, arg0);
    OnigRegion* region = match->onig_region;
    if ((index < 0) || (region->num_regs <= index)) {
        YogError_raise_IndexError(env, "no such group");
    }
    YogString* s = PTR_AS(YogString, match->str);
    YogVal body = s->body;
    const char* p = PTR_AS(YogCharArray, body)->items;
    const char* end = p + region->end[index] - 1;
    int_t n = ptr2index(env, s, end) + 1;

    RETURN(env, INT2VAL(n));
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
