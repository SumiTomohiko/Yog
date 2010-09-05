#if !defined(YOG_ENCODING_H_INCLUDED)
#define YOG_ENCODING_H_INCLUDED

#include "oniguruma.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/yog.h"

typedef uint_t (*YogGetCharBytes)(YogEnv*, YogHandle*, const char*);
typedef YogChar (*YogConvCharToYog)(YogEnv*, YogHandle*, const char*);
typedef uint_t (*YogGetYogCharBytes)(YogEnv*, YogHandle*, YogChar);
typedef uint_t (*YogConvCharFromYog)(YogEnv*, YogHandle*, YogChar, char*);

struct YogEncoding {
    struct YogBasicObj base;
    YogGetCharBytes get_char_bytes;
    YogConvCharToYog conv_char_to_yog;
    YogGetYogCharBytes get_yog_char_bytes;
    YogConvCharFromYog conv_char_from_yog;
};

typedef struct YogEncoding YogEncoding;

#define TYPE_ENCODING TO_TYPE(YogEncoding_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/encoding.c */
YOG_EXPORT YogVal YogEncoding_conv_from_yog(YogEnv*, YogHandle*, YogHandle*);
YOG_EXPORT YogVal YogEncoding_conv_to_yog(YogEnv*, YogHandle*, const char*, const char*);
YOG_EXPORT YogVal YogEncoding_create_ascii(YogEnv*);
YOG_EXPORT YogVal YogEncoding_create_euc_jp(YogEnv*);
YOG_EXPORT YogVal YogEncoding_create_shift_jis(YogEnv*);
YOG_EXPORT YogVal YogEncoding_create_utf8(YogEnv*);
YOG_EXPORT void YogEncoding_define_classes(YogEnv*, YogVal);
YOG_EXPORT YogVal YogEncoding_get_ascii(YogEnv*);
YOG_EXPORT YogVal YogEncoding_get_default(YogEnv*);
YOG_EXPORT YogVal YogEncoding_get_utf8(YogEnv*);
YOG_EXPORT YogVal YogEncoding_new(YogEnv*);
YOG_EXPORT YogVal YogEncoding_normalize_name(YogEnv*, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
