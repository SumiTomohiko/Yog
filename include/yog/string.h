#if !defined(__YOG_STRING_H__)
#define __YOG_STRING_H__

#include "yog/object.h"
#include "yog/yog.h"

struct YogCharArray {
    uint_t size;
    char items[0];
};

typedef struct YogCharArray YogCharArray;

struct YogString {
    YOGBASICOBJ_HEAD;
    YogVal encoding;
    uint_t size;
    YogVal body;
};

typedef struct YogString YogString;

#define TYPE_STRING     ((type_t)YogString_new)

#define STRING_CSTR(s)      PTR_AS(YogCharArray, PTR_AS(YogString, s)->body)->items
#define STRING_SIZE(s)      PTR_AS(YogString, s)->size
#define STRING_ENCODING(s)  PTR_AS(YogString, s)->encoding

#include "yog/encoding.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/string.c */
YOG_EXPORT YogVal YogCharArray_new_str(YogEnv*, const char*);
YOG_EXPORT void YogString_add(YogEnv*, YogVal, YogVal);
YOG_EXPORT void YogString_add_cstr(YogEnv*, YogVal, const char*);
YOG_EXPORT char YogString_at(YogEnv*, YogVal, uint_t);
YOG_EXPORT void YogString_clear(YogEnv*, YogVal);
YOG_EXPORT YogVal YogString_clone(YogEnv*, YogVal);
YOG_EXPORT void YogString_define_classes(YogEnv*, YogVal);
YOG_EXPORT char* YogString_dup(YogEnv*, const char*);
YOG_EXPORT void YogString_eval_builtin_script(YogEnv*, YogVal);
YOG_EXPORT YogVal YogString_from_range(YogEnv*, YogVal, const char*, const char*);
YOG_EXPORT YogVal YogString_from_str(YogEnv*, const char*);
YOG_EXPORT int_t YogString_hash(YogEnv*, YogVal);
YOG_EXPORT ID YogString_intern(YogEnv*, YogVal);
YOG_EXPORT YogVal YogString_multiply(YogEnv*, YogVal, int_t);
YOG_EXPORT YogVal YogString_new(YogEnv*);
YOG_EXPORT YogVal YogString_of_size(YogEnv*, uint_t);
YOG_EXPORT void YogString_push(YogEnv*, YogVal, char);
YOG_EXPORT uint_t YogString_size(YogEnv*, YogVal);
YOG_EXPORT YogVal YogString_to_i(YogEnv*, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
