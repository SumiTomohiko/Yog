#include "yog/yog.h"

YogRegexp* 
YogRegexp_new(YogEnv* env, YogString* pattern, OnigOptionType option) 
{
    OnigRegex onig_regexp = NULL;
    OnigUChar* pattern_begin = (OnigUChar*)pattern->body->items;
    OnigUChar* pattern_end = pattern_begin + pattern->body->size;
    OnigSyntaxType* syntax = ONIG_SYNTAX_DEFAULT;
    OnigErrorInfo einfo;

    int r = onig_new(&onig_regexp, pattern_begin, pattern_end, option, pattern->encoding->onig_enc, syntax, &einfo);
    if (r != ONIG_NORMAL) {
        return NULL;
    }

    YogRegexp* regexp = ALLOC_OBJ(env, NULL, YogRegexp);
    regexp->onig_regexp = onig_regexp;

    return regexp;
}

YogKlass* 
YogRegexp_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "Regexp", ENV_VM(env)->cObject);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
