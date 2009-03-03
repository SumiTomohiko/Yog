#include <string.h>
#include "oniguruma.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/regexp.h"
#include "yog/yog.h"

static void 
YogMatch_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicObj_keep_children(env, ptr, keeper);

    YogMatch* match = ptr;
#define KEEP(member)    match->member = (*keeper)(env, match->member)
    KEEP(str);
    KEEP(regexp);
#undef KEEP
}

static void 
YogMatch_finalize(YogEnv* env, void* ptr) 
{
    YogMatch* match = ptr;
    onig_region_free(match->onig_region, 1);
    match->onig_region = NULL;
}

YogMatch* 
YogMatch_new(YogEnv* env, YogString* str, YogRegexp* regexp, OnigRegion* onig_region) 
{
    YogMatch* match = ALLOC_OBJ(env, YogMatch_keep_children, YogMatch_finalize, YogMatch);
    YogBasicObj_init(env, YOGBASICOBJ(match), 0, ENV_VM(env)->cMatch);
    match->str = str;
    match->regexp = regexp;
    match->onig_region = onig_region;

    return match;
}

static void 
YogRegexp_finalize(YogEnv* env, void* ptr) 
{
    YogRegexp* regexp = ptr;
    onig_free(regexp->onig_regexp);
    regexp->onig_regexp = NULL;
}

YogVal 
YogRegexp_new(YogEnv* env, YogVal pattern, OnigOptionType option) 
{
    OnigRegex onig_regexp = NULL;
    OnigUChar* pattern_begin = (OnigUChar*)OBJ_AS(YogString, pattern)->body->items;
    OnigUChar* pattern_end = pattern_begin + OBJ_AS(YogString, pattern)->body->size - 1;
    OnigSyntaxType* syntax = ONIG_SYNTAX_DEFAULT;
    OnigErrorInfo einfo;

    int r = onig_new(&onig_regexp, pattern_begin, pattern_end, option, OBJ_AS(YogString, pattern)->encoding->onig_enc, syntax, &einfo);
    if (r != ONIG_NORMAL) {
        return YNIL;
    }

    YogRegexp* regexp = ALLOC_OBJ(env, NULL, YogRegexp_finalize, YogRegexp);
    YogBasicObj_init(env, YOGBASICOBJ(regexp), 0, ENV_VM(env)->cRegexp);
    regexp->onig_regexp = onig_regexp;

    return OBJ2VAL(regexp);
}

YogVal 
YogRegexp_klass_new(YogEnv* env) 
{
    return YogKlass_new(env, "Regexp", ENV_VM(env)->cObject);
}

static int 
group2index(YogEnv* env, YogMatch* match, YogVal arg)
{
    int index = 0;
    if (IS_OBJ_OF(cString, arg)) {
        YogString* s = OBJ_AS(YogString, arg);
        OnigRegex onig_regexp = match->regexp->onig_regexp;
        OnigUChar* name_begin = (OnigUChar*)s->body->items;
        OnigUChar* name_end = name_begin + s->body->size - 1;
        int* num_list = NULL;
        int r = onig_name_to_group_numbers(onig_regexp, name_begin, name_end, &num_list);
        YOG_ASSERT(env, r == 1, "TODO: index error?");
        index = num_list[0];
    }
    else if (IS_INT(arg)) {
        index = VAL2INT(arg);
    }
    else if (IS_NIL(arg)) {
        index = 0;
    }
    else {
        YOG_ASSERT(env, FALSE, "TODO: type error");
    }

    return index;
}

static YogVal 
group(YogEnv* env) 
{
    YogVal self = SELF(env);
    YogVal arg = ARG(env, 0);

    YogMatch* match = OBJ_AS(YogMatch, self);
    int index = group2index(env, match, arg);

    OnigRegion* region = match->onig_region;
    if ((index < 0) || (region->num_regs <= index)) {
        YOG_ASSERT(env, FALSE, "TODO: index error");
    }
    int begin = region->beg[index];
    int end = region->end[index];
    int size = end - begin;
    YogVal s = YogString_new_size(env, size + 1);
    memcpy(OBJ_AS(YogString, s)->body->items, &match->str->body->items[begin], size);
    OBJ_AS(YogString, s)->body->items[size] = '\0';

    return s;
}

static int 
ptr2index(YogEnv* env, YogString* s, const char* ptr) 
{
    unsigned int index = 0;
    YogEncoding* enc = s->encoding;
    const char* p = s->body->items;
    while (p < ptr) {
        p += YogEncoding_mbc_size(env, enc, p);
        index++;
    }

    return index;
}

static YogVal 
start(YogEnv* env) 
{
    YogVal self = SELF(env);
    YogVal arg = ARG(env, 0);

    YogMatch* match = OBJ_AS(YogMatch, self);
    int index = group2index(env, match, arg);
    OnigRegion* region = match->onig_region;
    if ((index < 0) || (region->num_regs <= index)) {
        YOG_ASSERT(env, FALSE, "TODO: index error");
    }
    YogString* s = match->str;
    const char* start = s->body->items + region->beg[index];
    int n = ptr2index(env, s, start);

    return INT2VAL(n);
}

static YogVal 
end(YogEnv* env) 
{
    YogVal self = SELF(env);
    YogVal arg = ARG(env, 0);

    YogMatch* match = OBJ_AS(YogMatch, self);
    int index = group2index(env, match, arg);
    OnigRegion* region = match->onig_region;
    if ((index < 0) || (region->num_regs <= index)) {
        YOG_ASSERT(env, FALSE, "TODO: index error");
    }
    YogString* s = match->str;
    const char* end = s->body->items + region->end[index] - 1;
    int n = ptr2index(env, s, end) + 1;

    return INT2VAL(n);
}

YogVal 
YogMatch_klass_new(YogEnv* env) 
{
    YogVal klass = YogKlass_new(env, "Match", ENV_VM(env)->cObject);
    YogKlass_define_method(env, klass, "group", group, 0, 0, 0, 0, "group", NULL);
    YogKlass_define_method(env, klass, "start", start, 0, 0, 0, 0, "group", NULL);
    YogKlass_define_method(env, klass, "end", end, 0, 0, 0, 0, "group", NULL);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
