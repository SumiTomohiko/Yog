#include "yog/config.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "oniguruma.h"
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/string.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"

YogVal
YogEncoding_get_ascii(YogEnv* env)
{
    return env->vm->encAscii;
}

YogVal
YogEncoding_get_utf8(YogEnv* env)
{
    return env->vm->encUtf8;
}

YogVal
YogEncoding_get_default(YogEnv* env)
{
    return YogEncoding_get_utf8(env);
}

YogVal
YogEncoding_normalize_name(YogEnv* env, YogVal name)
{
    YogVal s = YogString_clone(env, name);
    uint_t size = STRING_SIZE(s);
    uint_t i;
    for (i = 0; i < size; i++) {
        YogChar c = STRING_CHARS(s)[i];
        STRING_CHARS(s)[i] = c == '_' ? '-' : tolower(c);
    }

    return s;
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    YogVal enc = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, YogEncoding);
    YogBasicObj_init(env, enc, TYPE_ENCODING, 0, klass);
    return enc;
}

YogVal
YogEncoding_new(YogEnv* env)
{
    YogVal enc = alloc(env, env->vm->cEncoding);
    PTR_AS(YogEncoding, enc)->get_char_bytes = NULL;
    PTR_AS(YogEncoding, enc)->conv_char_to_yog = NULL;
    PTR_AS(YogEncoding, enc)->get_yog_char_bytes = NULL;
    PTR_AS(YogEncoding, enc)->conv_char_from_yog = NULL;
    return enc;
}

static YogVal
create_string(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "create_string", params, args, kw);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_ENCODING)) {
        YogError_raise_TypeError((env), "self must be Encoding");
    }
    RETURN(env, YogString_new(env));
}

void
YogEncoding_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cEncoding = YUNDEF;
    PUSH_LOCAL(env, cEncoding);
    YogVM* vm = env->vm;

    cEncoding = YogClass_new(env, "Encoding", vm->cObject);
    YogClass_define_allocator(env, cEncoding, alloc);
    YogClass_define_method(env, cEncoding, pkg, "create_string", create_string);
    vm->cEncoding = cEncoding;

    RETURN_VOID(env);
}

static uint_t
count_chars(YogEnv* env, YogHandle* self, const char* begin, const char* end)
{
    uint_t n = 0;
    const char* pc = begin;
    while (pc < end) {
        n++;
        pc += HDL_AS(YogEncoding, self)->get_char_bytes(env, self, pc);
    }
    return n;
}

static uint_t
compute_bytes(YogEnv* env, YogHandle* self, YogHandle* s)
{
    uint_t n = 0;
    uint_t size = STRING_SIZE(HDL2VAL(s));
    uint_t i;
    for (i = 0; i < size; i++) {
        YogChar c = STRING_CHARS(HDL2VAL(s))[i];
        n += HDL_AS(YogEncoding, self)->get_yog_char_bytes(env, self, c);
    }
    return n;
}

YogVal
YogEncoding_conv_from_yog(YogEnv* env, YogHandle* self, YogHandle* s)
{
    uint_t bytes_num = compute_bytes(env, self, s);
    YogVal bin = YogBinary_of_size(env, bytes_num + 1);
    char* pc = BINARY_CSTR(bin);
    uint_t size = STRING_SIZE(HDL2VAL(s));
    uint_t i;
    for (i = 0; i < size; i++) {
        YogConvCharFromYog conv = HDL_AS(YogEncoding, self)->conv_char_from_yog;
        pc += conv(env, self, STRING_CHARS(HDL2VAL(s))[i], pc);
    }
    *pc = '\0';

    return bin;
}

YogVal
YogEncoding_conv_to_yog(YogEnv* env, YogHandle* self, const char* begin, const char* end)
{
    uint_t chars_num = count_chars(env, self, begin, end);
    YogVal s = YogString_of_size(env, chars_num);
    uint_t i;
    const char* pc = begin;
    for (i = 0; i < chars_num; i++) {
        YogChar c = HDL_AS(YogEncoding, self)->conv_char_to_yog(env, self, pc);
        STRING_CHARS(s)[i] = c;
        pc += HDL_AS(YogEncoding, self)->get_char_bytes(env, self, pc);
    }
    STRING_SIZE(s) = chars_num;

    return s;
}

static uint_t
shift_jis_get_char_bytes(YogEnv* env, YogHandle* self, const char* s)
{
    unsigned char c = (unsigned char)s[0];
    if ((0x81 <= c) && (c <= 0x9f)) {
        return 2;
    }
    if ((0xe0 <= c) && (c <= 0xfc)) {
        return 2;
    }
    return 1;
}

static YogChar
shift_jis_conv_char_to_yog(YogEnv* env, YogHandle* self, const char* s)
{
    unsigned char c1 = (unsigned char)s[0];
    if (isascii(c1)) {
        return c1;
    }
    if ((0xa1 <= c1) && (c1 <= 0xdf)) {
        YogChar table[] = {
#include "shift_jis2yog_hankaku.inc"
        };
        return table[c1 - 0xa1];
    }
    uint_t n;
    if ((0x81 <= c1) && (c1 <= 0x9f)) {
        n = c1 - 0x81;
    }
    else if ((0xe0 <= c1) && (c1 <= 0xfc)) {
        n = c1 - 0xc0 + 0x9f - 0x81 + 1;
    }
    else {
        return '?';
    }
    unsigned char c2 = (unsigned char)s[1];
    uint_t m;
    if ((0x40 <= c2) && (c2 <= 0x7e)) {
        m = c2 - 0x40;
    }
    else if ((0x80 <= c2) && (c2 <= 0xfc)) {
        m = c2 - 0x80 + 0x7e - 0x40 + 1;
    }
    else {
        return '?';
    }
    YogChar shift_jis2yog_zenkaku[] = {
#include "shift_jis2yog_zenkaku.inc"
    };
    uint_t index = ((0x7e - 0x40 + 1) + (0xfc - 0x80 + 1)) * n + m;
    return shift_jis2yog_zenkaku[index];
}

static uint_t
euc_jp_get_char_bytes(YogEnv* env, YogHandle* self, const char* s)
{
    unsigned char c = (unsigned char)s[0];
    if (((0xa1 <= c) && (c <= 0xfe)) || (c == 0x8e)) {
        return 2;
    }
    if (c == 0x8f) {
        return 3;
    }
    return 1;
}

static YogChar
euc_jp_conv_char_to_yog(YogEnv* env, YogHandle* self, const char* s)
{
    unsigned char c1 = (unsigned char)s[0];
    if (isascii(c1)) {
        return c1;
    }
    if (c1 == 0x8e) {
        YogChar euc_jp2yog_hankaku[] = {
#include "euc_jp2yog_hankaku.inc"
        };
        return euc_jp2yog_hankaku[s[1] - 0xa1];
    }
    if (c1 == 0x8f) {
        YogChar euc_jp2yog_3bytes[] = {
#include "euc_jp2yog_3bytes.inc"
        };
        uint_t n = s[1] - 0xa1;
        uint_t m = s[2] - 0xa1;
        return euc_jp2yog_3bytes[(0xfe - 0xa1 + 1) * n + m];
    }
    uint_t n = c1 - 0xa1;
    uint_t m = (0xff & s[1]) - 0xa1;
    YogChar euc_jp2yog_zenkaku[] = {
#include "euc_jp2yog_zenkaku.inc"
    };
    return euc_jp2yog_zenkaku[(0xfe - 0xa1 + 1) * n + m];
}

static uint_t
utf8_get_char_bytes(YogEnv* env, YogHandle* self, const char* s)
{
#define RETURN_IF_MATCH(x, v) do { \
    if ((s[0] & (x)) == (x)) { \
        return (v); \
    } \
} while (0)
    RETURN_IF_MATCH(0xfc, 6);
    RETURN_IF_MATCH(0xf8, 5);
    RETURN_IF_MATCH(0xf0, 4);
    RETURN_IF_MATCH(0xe0, 3);
    RETURN_IF_MATCH(0xc0, 2);
#undef RETURN_IF_MATCH
    return 1;
}

static YogChar
utf8_conv_char_to_yog(YogEnv* env, YogHandle* self, const char* s)
{
#define GET_1_BIT(x)    ((x) & 0x01)
#define GET_2_BITS(x)   ((x) & 0x03)
#define GET_3_BITS(x)   ((x) & 0x07)
#define GET_4_BITS(x)   ((x) & 0x0f)
#define GET_5_BITS(x)   ((x) & 0x1f)
#define GET_6_BITS(x)   ((x) & 0x3f)
    switch (utf8_get_char_bytes(env, self, s)) {
    case 1:
        return s[0];
    case 2:
        return (GET_5_BITS(s[0]) << 6) + GET_6_BITS(s[1]);
    case 3:
        return (GET_4_BITS(s[0]) << 12) + (GET_6_BITS(s[1]) << 6) + GET_6_BITS(s[2]);
    case 4:
        return (GET_3_BITS(s[0]) << 18) + (GET_6_BITS(s[1]) << 12) + (GET_6_BITS(s[2]) << 12) + GET_6_BITS(s[3]);
    case 5:
        return (GET_2_BITS(s[0]) << 24) + (GET_6_BITS(s[1]) << 18) + (GET_6_BITS(s[2]) << 12) + (GET_6_BITS(s[3]) << 6) + GET_6_BITS(s[4]);
    case 6:
        return (GET_1_BIT(s[0]) << 30) + (GET_2_BITS(s[1]) << 24) + (GET_6_BITS(s[2]) << 18) + (GET_6_BITS(s[3]) << 12) + (GET_6_BITS(s[4]) << 6) + GET_6_BITS(s[5]);
    default:
        return '?';
    }
#undef GET_6_BITS
#undef GET_5_BITS
#undef GET_4_BITS
#undef GET_3_BITS
#undef GET_2_BITS
#undef GET_1_BIT
}

static uint_t
ascii_get_char_bytes(YogEnv* env, YogHandle* self, const char* s)
{
    return 1;
}

static YogChar
ascii_conv_char_to_yog(YogEnv* env, YogHandle* self, const char* s)
{
    return *s;
}

static uint_t
utf8_get_yog_char_bytes(YogEnv* env, YogHandle* self, YogChar c)
{
    if (isascii(c)) {
        return 1;
    }
    if (c < 0x800) {
        return 2;
    }
    if (c < 0x10000) {
        return 3;
    }
    if (c < 0x200000) {
        return 4;
    }
    if (c < 0x4000000) {
        return 5;
    }
    return 6;
}

static uint_t
utf8_conv_char_from_yog(YogEnv* env, YogHandle* self, YogChar c , char* dest)
{
    if (isascii(c)) {
        dest[0] = c;
        return 1;
    }
    if (c < 0x800) {
        dest[0] = 0xc0 | (c >> 6);
        dest[1] = 0x80 | (c & 0x3f);
        return 2;
    }
    if (c < 0x10000) {
        dest[0] = 0xe0 | (c >> 12);
        dest[1] = 0x80 | ((c >> 6) & 0x3f);
        dest[2] = 0x80 | (c & 0x3f);
        return 3;
    }
    if (c < 0x200000) {
        dest[0] = 0xf0 | (c >> 18);
        dest[1] = 0x80 | ((c >> 12) & 0x3f);
        dest[2] = 0x80 | ((c >> 6) & 0x3f);
        dest[3] = 0x80 | (c & 0x3f);
        return 4;
    }
    if (c < 0x4000000) {
        dest[0] = 0xf8 | (c >> 24);
        dest[1] = 0x80 | ((c >> 18) & 0x3f);
        dest[2] = 0x80 | ((c >> 12) & 0x3f);
        dest[3] = 0x80 | ((c >> 6) & 0x3f);
        dest[4] = 0x80 | (c & 0x3f);
        return 5;
    }
    dest[0] = 0xfc | (c >> 30);
    dest[1] = 0x80 | ((c >> 24) & 0x3f);
    dest[2] = 0x80 | ((c >> 18) & 0x3f);
    dest[3] = 0x80 | ((c >> 12) & 0x3f);
    dest[4] = 0x80 | ((c >> 6) & 0x3f);
    dest[5] = 0x80 | (c & 0x3f);
    return 6;
}

YogVal
YogEncoding_create_utf8(YogEnv* env)
{
    YogVal enc = YogEncoding_new(env);
    PTR_AS(YogEncoding, enc)->get_char_bytes = utf8_get_char_bytes;
    PTR_AS(YogEncoding, enc)->conv_char_to_yog = utf8_conv_char_to_yog;
    PTR_AS(YogEncoding, enc)->get_yog_char_bytes = utf8_get_yog_char_bytes;
    PTR_AS(YogEncoding, enc)->conv_char_from_yog = utf8_conv_char_from_yog;
    return enc;
}

struct TableEntry {
    YogChar c;
    const char* s;
};

typedef struct TableEntry TableEntry;

static const char*
binary_search(YogEnv*env, YogHandle* self, TableEntry* table, uint_t min, uint_t max, YogChar c)
{
    if (max < min) {
        return NULL;
    }
    uint_t pos = (min + max) / 2;
    if (table[pos].c == c) {
        return table[pos].s;
    }
    if (table[pos].c < c) {
        return binary_search(env, self, table, pos + 1, max, c);
    }
    return binary_search(env, self, table, min, pos - 1, c);
}

static const char*
yog2shift_jis(YogEnv* env, YogHandle* self, YogChar c)
{
    if ((0xa0 <= c) && (c <= 0x1ff)) {
        const char* table[] = {
#include "yog2shift_jis1.inc"
        };
        return table[c - 0xa0];
    }
    if ((0x250 <= c) && (c <= 0x451)) {
        const char* table[] = {
#include "yog2shift_jis2.inc"
        };
        return table[c - 0x250];
    }
    if ((c < 0x1e3e) || (0x2a6b2 < c)) {
        return "?";
    }
    TableEntry table[] = {
#include "yog2shift_jis3.inc"
    };
    return binary_search(env, self, table, 0, YOG2SHIFT_JIS3_NUM - 1, c);
}

static uint_t
shift_jis_get_yog_char_bytes(YogEnv* env, YogHandle* self, YogChar c)
{
    return strlen(yog2shift_jis(env, self, c));
}

static uint_t
copy_string(char* dest, const char* src)
{
    uint_t i;
    for (i = 0; src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    return i;
}

static uint_t
shift_jis_conv_char_from_yog(YogEnv* env, YogHandle* self, YogChar c, char* dest)
{
    return copy_string(dest, yog2shift_jis(env, self, c));
}

YogVal
YogEncoding_create_shift_jis(YogEnv* env)
{
    YogVal enc = YogEncoding_new(env);
    PTR_AS(YogEncoding, enc)->get_char_bytes = shift_jis_get_char_bytes;
    PTR_AS(YogEncoding, enc)->conv_char_to_yog = shift_jis_conv_char_to_yog;
    PTR_AS(YogEncoding, enc)->get_yog_char_bytes = shift_jis_get_yog_char_bytes;
    PTR_AS(YogEncoding, enc)->conv_char_from_yog = shift_jis_conv_char_from_yog;
    return enc;
}

static const char*
yog2euc_jp(YogEnv* env, YogHandle* self, YogChar c)
{
    if ((0xa0 <= c) && (c <= 0x1ff)) {
        const char* table[] = {
#include "yog2euc_jp1.inc"
        };
        return table[c - 0xa0];
    }
    if ((0x250 <= c) && (c <= 0x451)) {
        const char* table[] = {
#include "yog2euc_jp2.inc"
        };
        return table[c - 0x250];
    }
    if ((c < 0x1e3e) || (0x2a6b2 < c)) {
        return "?";
    }
    TableEntry table[] = {
#include "yog2euc_jp3.inc"
    };
    return binary_search(env, self, table, 0, YOG2EUC_JP3_NUM - 1, c);
}

static uint_t
euc_jp_conv_char_from_yog(YogEnv* env, YogHandle* self, YogChar c, char* dest)
{
    return copy_string(dest, yog2euc_jp(env, self, c));
}

static uint_t
euc_jp_get_yog_char_bytes(YogEnv* env, YogHandle* self, YogChar c)
{
    return strlen(yog2euc_jp(env, self, c));
}

YogVal
YogEncoding_create_euc_jp(YogEnv* env)
{
    YogVal enc = YogEncoding_new(env);
    PTR_AS(YogEncoding, enc)->get_char_bytes = euc_jp_get_char_bytes;
    PTR_AS(YogEncoding, enc)->conv_char_to_yog = euc_jp_conv_char_to_yog;
    PTR_AS(YogEncoding, enc)->get_yog_char_bytes = euc_jp_get_yog_char_bytes;
    PTR_AS(YogEncoding, enc)->conv_char_from_yog = euc_jp_conv_char_from_yog;
    return enc;
}

static uint_t
ascii_conv_char_from_yog(YogEnv* env, YogHandle* self, YogChar c , char* dest)
{
    *dest = isascii(c) ? c : '?';
    return 1;
}

static uint_t
ascii_get_yog_char_bytes(YogEnv* env, YogHandle* self, YogChar c)
{
    return 1;
}

YogVal
YogEncoding_create_ascii(YogEnv* env)
{
    YogVal enc = YogEncoding_new(env);
    PTR_AS(YogEncoding, enc)->get_char_bytes = ascii_get_char_bytes;
    PTR_AS(YogEncoding, enc)->conv_char_to_yog = ascii_conv_char_to_yog;
    PTR_AS(YogEncoding, enc)->get_yog_char_bytes = ascii_get_yog_char_bytes;
    PTR_AS(YogEncoding, enc)->conv_char_from_yog = ascii_conv_char_from_yog;
    return enc;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
