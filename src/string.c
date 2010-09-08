#include "yog/config.h"
#include <ctype.h>
#include <errno.h>
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oniguruma.h"
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/binary.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/regexp.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRING)) { \
        YogError_raise_TypeError((env), "self must be String"); \
    } \
} while (0)
#define CHECK_SELF_TYPE2(env, self)  do { \
    YogVal s = HDL2VAL((self)); \
    if (!IS_PTR(s) || (BASIC_OBJ_TYPE(s) != TYPE_STRING)) { \
        YogError_raise_TypeError((env), "self must be String"); \
    } \
} while (0)

ID
YogString_intern(YogEnv* env, YogVal s)
{
    return YogVM_intern2(env, env->vm, s);
}

static YogVal
YogCharArray_new(YogEnv* env, uint_t size)
{
    YogGC_check_multiply_overflow(env, size, sizeof(YogChar));
    YogVal array = ALLOC_OBJ_ITEM(env, NULL, NULL, YogCharArray, size, YogChar);
    PTR_AS(YogCharArray, array)->size = size;

    return array;
}

static void
YogString_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogString* s = PTR_AS(YogString, ptr);
#define KEEP(member)    YogGC_KEEP(env, s, member, keeper, heap)
    KEEP(body);
#undef KEEP
}

uint_t
YogString_size(YogEnv* env, YogVal string)
{
    return PTR_AS(YogString, string)->size;
}

static void
ensure_body(YogEnv* env, YogVal string)
{
    SAVE_ARG(env, string);

    if (!IS_PTR(PTR_AS(YogString, string)->body)) {
#define CAPACITY    (1)
        YogVal body = YogCharArray_new(env, CAPACITY);
#undef CAPACITY
        YogGC_UPDATE_PTR(env, PTR_AS(YogString, string), body, body);
    }

    RETURN_VOID(env);
}

static void
ensure_size(YogEnv* env, YogVal string, uint_t needed_size)
{
    SAVE_ARG(env, string);
    ensure_body(env, string);
    YogVal body = PTR_AS(YogString, string)->body;
    PUSH_LOCAL(env, body);
    if (needed_size <= PTR_AS(YogCharArray, body)->size) {
        RETURN_VOID(env);
    }

    uint_t capacity = IS_PTR(body) ? 1 : PTR_AS(YogCharArray, body)->size;
    do {
#define RATIO 2
        capacity = RATIO * capacity;
#undef RATIO
    } while (capacity < needed_size);

    YogVal new_body = YogCharArray_new(env, capacity);
    YogChar* p = PTR_AS(YogCharArray, new_body)->items;
    YogChar* q = PTR_AS(YogCharArray, body)->items;
    uint_t size = sizeof(YogChar) * STRING_SIZE(string);
    memcpy(p, q, size);

    YogGC_UPDATE_PTR(env, PTR_AS(YogString, string), body, new_body);

    RETURN_VOID(env);
}

void
YogString_push(YogEnv* env, YogVal self, YogChar c)
{
    YogHandle* s = VAL2HDL(env, self);
    uint_t n = STRING_SIZE(self);
    uint_t needed_size = n + 1;
    ensure_size(env, self, needed_size);
    STRING_CHARS(HDL2VAL(s))[n] = c;
    STRING_SIZE(HDL2VAL(s)) = needed_size;
}

void
YogString_clear(YogEnv* env, YogVal self)
{
    STRING_SIZE(self) = 0;
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogVal obj = ALLOC_OBJ(env, YogString_keep_children, NULL, YogString);
    YogBasicObj_init(env, obj, TYPE_STRING, 0, klass);
    PTR_AS(YogString, obj)->size = 0;
    PTR_AS(YogString, obj)->body = YUNDEF;

    RETURN(env, obj);
}

YogVal
YogString_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal self = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS2(env, self, body);

    self = alloc(env, env->vm->cString);
    body = YogCharArray_new(env, 1);
    YogGC_UPDATE_PTR(env, PTR_AS(YogString, self), body, body);
    PTR_AS(YogString, self)->size = 0;

    RETURN(env, self);
}

YogVal
YogString_from_range(YogEnv* env, YogVal enc, const char* start, const char* end)
{
    SAVE_ARG(env, enc);

    uint_t size = 0;
    if (start <= end) {
        size = end - start + 1;
    }

    /* FIXME: dirty hack */
    char* escaped_from_gc = (char*)YogSysdeps_alloca(sizeof(char) * size);
    strncpy(escaped_from_gc, start, size);

    YogVal body = YogCharArray_new(env, size + 1);
    memcpy(PTR_AS(YogCharArray, body)->items, escaped_from_gc, size);
    PTR_AS(YogCharArray, body)->items[size] = '\0';
    PUSH_LOCAL(env, body);

    YogVal s = YogString_new(env);
    PTR_AS(YogString, s)->size = size + 1;
    YogGC_UPDATE_PTR(env, PTR_AS(YogString, s), body, body);

    RETURN(env, s);
}

YogVal
YogString_of_size(YogEnv* env, uint_t size)
{
    SAVE_LOCALS(env);
    YogVal string = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS2(env, string, body);

    string = alloc(env, env->vm->cString);
    if (size == 0) {
        RETURN(env, string);
    }
    body = YogCharArray_new(env, size);

    STRING_SIZE(string) = 0;
    YogGC_UPDATE_PTR(env, PTR_AS(YogString, string), body, body);

    RETURN(env, string);
}

YogVal
YogString_from_string(YogEnv* env, const char* s)
{
    YogHandle* enc = YogHandle_REGISTER(env, env->vm->encUtf8);
    return YogEncoding_conv_to_yog(env, enc, s, s + strlen(s));
}

YogVal
YogString_clone(YogEnv* env, YogVal self)
{
    YogHandle* h = VAL2HDL(env, self);
    uint_t size = STRING_SIZE(self);
    YogVal s = YogString_of_size(env, size);
    memcpy(STRING_CHARS(s), STRING_CHARS(HDL2VAL(h)), sizeof(YogChar) * size);
    STRING_SIZE(s) = size;
    return s;
}

static int_t
strstr_internal(YogEnv* env, YogVal self, int_t pos, const char* substr)
{
    YogChar* p = STRING_CHARS(self) + pos;
    YogChar* pend = STRING_CHARS(self) + STRING_SIZE(self);
    while ((p < pend) && (*p != substr[0])) {
        p++;
    };
    if (p == pend) {
        return -1;
    }
    int_t index = p - STRING_CHARS(self);
    const char* q = substr;
    while ((p < pend) && (*q != '\0') && (*p == *q)) {
        p++;
        q++;
    }
    if (*q == '\0') {
        return index;
    }
    return strstr_internal(env, self, index + 1, substr);
}

int_t
YogString_strstr(YogEnv* env, YogVal self, const char* substr)
{
    return strstr_internal(env, self, 0, substr);
}

int_t
YogString_strncmp(YogEnv* env, YogVal self, YogVal s, uint_t size)
{
    uint_t i;
    for (i = 0; i < size; i++) {
        if (STRING_CHARS(self)[i] != STRING_CHARS(s)[i]) {
            return STRING_CHARS(self)[i] - STRING_CHARS(s)[i];
        }
    }
    return 0;
}

static uint_t
find(YogEnv* env, YogVal self, YogVal substr, uint_t from)
{
    SAVE_ARGS2(env, self, substr);

    uint_t self_size = YogString_size(env, self);
    uint_t substr_size = YogString_size(env, substr);
    uint_t end_pos = self_size - substr_size;
    if (self_size < end_pos) {
        RETURN(env, UNSIGNED_MAX);
    }

    uint_t i;
    for (i = from; i <= end_pos; i++) {
        uint_t j;
        for (j = 0; j < substr_size; j++) {
            if (STRING_CHARS(self)[i + j] != STRING_CHARS(substr)[j]) {
                break;
            }
        }
        if (j == substr_size) {
            RETURN(env, i);
        }
    }

    RETURN(env, UNSIGNED_MAX);
}

void
YogString_append(YogEnv* env, YogVal self, YogVal s)
{
    SAVE_ARGS2(env, self, s);

    uint_t self_size = YogString_size(env, self);
    uint_t s_size = YogString_size(env, s);
    uint_t size = self_size + s_size;
    ensure_size(env, self, size);
    YogChar* dest = STRING_CHARS(self) + self_size;
    memcpy(dest, STRING_CHARS(s), sizeof(YogChar) * s_size);
    STRING_SIZE(self) = size;

    RETURN_VOID(env);
}

static YogVal
gsub(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal substr = YUNDEF;
    YogVal s = YUNDEF;
    YogVal to = YUNDEF;
    PUSH_LOCALS3(env, substr, s, to);
    YogCArg params[] = { { "substr", &substr }, { "to", &to }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "gsub", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    if (!IS_PTR(substr) || (BASIC_OBJ_TYPE(substr) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "substring must be String");
    }
    if (!IS_PTR(to) || (BASIC_OBJ_TYPE(to) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "replacing string must be String");
    }

#define ADD_STR(to) do { \
    uint_t size = (to) - from; \
    YogVal t = YogString_of_size(env, size); \
    YogChar* dest = STRING_CHARS(t); \
    memcpy(dest, STRING_CHARS(self) + from, sizeof(YogChar) * size); \
    STRING_SIZE(t) = size; \
    YogString_append(env, s, t); \
} while (0)
    s = YogString_new(env);
    uint_t from = 0;
    uint_t index;
    while ((index = find(env, self, substr, from)) != UNSIGNED_MAX) {
        ADD_STR(index);
        YogString_append(env, s, to);
        from = index + YogString_size(env, substr);
    }
    ADD_STR(STRING_SIZE(self));
#undef ADD_STR

    RETURN(env, s);
}

static YogVal
get_size(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    CHECK_SELF_TYPE(env, self);
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_size", params, args, kw);
    RETURN(env, INT2VAL(STRING_SIZE(self)));
}

static YogVal
to_i(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal n = YUNDEF;
    PUSH_LOCAL(env, n);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_i", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    n = YogString_to_i(env, self);

    RETURN(env, n);
}

static YogVal
to_bin(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* encoding)
{
    CHECK_SELF_TYPE2(env, self);
    YogMisc_check_encoding(env, encoding, "encoding");
    return YogEncoding_conv_from_yog(env, encoding, self);
}

static YogVal
to_s(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_s", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    RETURN(env, self);
}

YogVal
YogString_binop_add(YogEnv* env, YogHandle* self, YogHandle* s)
{
    YogVal right = HDL2VAL(s);
    if (!IS_PTR(right) || (BASIC_OBJ_TYPE(right) != TYPE_STRING)) {
        YogError_raise_binop_type_error(env, HDL2VAL(self), right, "+");
        /* NOTREACHED */
    }

    uint_t size1 = STRING_SIZE(HDL2VAL(self));
    uint_t size2 = STRING_SIZE(right);
    uint_t size = size1 + size2;
    YogVal t = YogString_of_size(env, size);
    const YogChar* p = STRING_CHARS(HDL2VAL(self));
    memcpy(STRING_CHARS(t), p, sizeof(YogChar) * size1);
    const YogChar* q = STRING_CHARS(HDL2VAL(s));
    memcpy(STRING_CHARS(t) + size1, q, sizeof(YogChar) * size2);
    STRING_SIZE(t) = size;

    return t;
}

static YogVal
add(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* s)
{
    CHECK_SELF_TYPE2(env, self);
    if (!IS_PTR(HDL2VAL(s)) || (BASIC_OBJ_TYPE(HDL2VAL(s)) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "Can't convert %C object to string implicitly", HDL2VAL(s));
        /* NOTREACHED */
    }
    return YogString_binop_add(env, self, s);
}

YogVal
YogString_binop_multiply(YogEnv* env, YogHandle* self, YogVal n)
{
    if (!IS_FIXNUM(n)) {
        const char* fmt = "Can't multiply string by non-Fixnum of type %C";
        YogError_raise_TypeError(env, fmt, n);
        /* NOTREACHED */
    }

    uint_t size = STRING_SIZE(HDL2VAL(self));
    int_t num = VAL2INT(n);
    if (num < 0) {
        YogError_raise_ArgumentError(env, "Negative argument");
        /* NOTREACHED */
    }
    uint_t needed_size = size * num;
    if ((num != 0) && (needed_size / num != size)) {
        YogError_raise_ArgumentError(env, "Argument too big");
        /* NOTREACHED */
    }
    YogVal s = YogString_of_size(env, needed_size);
    int_t i;
    for (i = 0; i < num; i++) {
        uint_t n = sizeof(YogChar) * size;
        memcpy(STRING_CHARS(s) + i * size, STRING_CHARS(HDL2VAL(self)), n);
    }
    STRING_SIZE(s) = needed_size;

    return s;
}

static YogVal
multiply(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogString_binop_multiply(env, self, HDL2VAL(n));
}

YogVal
YogString_binop_lshift(YogEnv* env, YogHandle* self, YogHandle* s)
{
    if (!IS_PTR(HDL2VAL(s)) || (BASIC_OBJ_TYPE(HDL2VAL(s)) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "Operand must be String");
    }

    uint_t size1 = YogString_size(env, HDL2VAL(self));
    uint_t size2 = YogString_size(env, HDL2VAL(s));
    uint_t size = size1 + size2;
    ensure_size(env, HDL2VAL(self), size);
    YogChar* dest = STRING_CHARS(HDL2VAL(self)) + size1;
    memcpy(dest, STRING_CHARS(HDL2VAL(s)), sizeof(YogChar) * size2);
    STRING_SIZE(HDL2VAL(self)) = size;

    return HDL2VAL(self);
}

static YogVal
lshift(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* s)
{
    CHECK_SELF_TYPE2(env, self);
    return YogString_binop_lshift(env, self, s);
}

static BOOL
index2offset(YogEnv* env, YogVal self, int_t index, uint_t* offset)
{
    if (index < 0) {
        return FALSE;
    }
    if (index < STRING_SIZE(self)) {
        *offset = index;
        return TRUE;
    }
    return FALSE;
}

static int_t
normalize_index(YogEnv* env, YogVal self, int_t index)
{
    return 0 <= index ? index : STRING_SIZE(self) + index;
}

static uint_t
unnormalized_index2offset(YogEnv* env, YogVal self, int_t index)
{
    SAVE_ARG(env, self);

    int_t n = normalize_index(env, self, index);
    uint_t offset = 0;
    if (!index2offset(env, self, n, &offset)) {
        YogError_raise_IndexError(env, "String index out of range");
    }

    RETURN(env, offset);
}

static YogVal
get_at(YogEnv* env, YogVal self, int_t offset)
{
    SAVE_ARG(env, self);
    YogVal c = YogString_of_size(env, 1);
    PUSH_LOCAL(env, c);

    STRING_CHARS(c)[0] = STRING_CHARS(self)[offset];
    STRING_SIZE(c) = 1;

    RETURN(env, c);
}

static YogVal
get(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal index = YUNDEF;
    YogVal default_ = YUNDEF;
    YogVal c = YUNDEF;
    PUSH_LOCALS3(env, index, default_, c);
    YogCArg params[] = {
        { "index", &index },
        { "|", NULL },
        { "default", &default_ },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "get", params, args, kw);
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "String index must be Fixnum");
    }

    int_t n = normalize_index(env, self, VAL2INT(index));
    uint_t offset = 0;
    if (!index2offset(env, self, n, &offset)) {
        if (!IS_UNDEF(default_)) {
            RETURN(env, default_);
        }
        YogError_raise_IndexError(env, "String index out of range");
    }

    c = get_at(env, self, offset);
    RETURN(env, c);
}

YogVal
YogString_subscript(YogEnv* env, YogVal self, YogVal index)
{
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "String index must be Fixnum");
    }

    uint_t offset = unnormalized_index2offset(env, self, VAL2INT(index));
    return get_at(env, self, offset);
}

static int_t
normalize_position(YogEnv* env, int_t pos, int_t max_index)
{
    return pos < 0 ? pos + max_index + 1 : pos;
}

static int_t
normalize_length(YogEnv* env, YogHandle* len, int_t max_index, int_t pos)
{
    if ((len == NULL) || (max_index < pos + VAL2INT(HDL2VAL(len)))) {
        return max_index - pos + 1;
    }
    return VAL2INT(HDL2VAL(len));
}

YogVal
YogString_slice(YogEnv* env, YogHandle* self, uint_t pos, uint_t size)
{
    YogVal s = YogString_of_size(env, size);
    const YogChar* src = STRING_CHARS(HDL2VAL(self)) + pos;
    memcpy(STRING_CHARS(s), src, sizeof(YogChar) * size);
    STRING_SIZE(s) = size;
    return s;
}

static YogVal
slice(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* pos, YogHandle* len)
{
    CHECK_SELF_TYPE2(env, self);
    int_t max_index = STRING_SIZE(HDL2VAL(self)) - 1;
    if (!IS_FIXNUM(HDL2VAL(pos))) {
        const char* fmt = "pos must be Fixnum, not %C";
        YogError_raise_TypeError(env, fmt, HDL2VAL(pos));
    }
    int_t n = normalize_position(env, VAL2INT(HDL2VAL(pos)), max_index);
    if ((n < 0) || (max_index < n)) {
        return YogString_new(env);
    }
    uint_t begin;
    if (!index2offset(env, HDL2VAL(self), n, &begin)) {
        const char* fmt = "Position %d out of range [-%u, %u]";
        int_t n = VAL2INT(HDL2VAL(pos));
        uint_t size = STRING_SIZE(HDL2VAL(self));
        uint_t lower = size;
        uint_t upper = size - 1;
        YogError_raise_IndexError(env, fmt, n, lower, upper);
        /* NOTREACHED */
    }
    if ((len != NULL) && (VAL2INT(HDL2VAL(len)) <= 0)) {
        return YogString_new(env);
    }
    int_t l = normalize_length(env, len, max_index, n);
    uint_t end;
    if (!index2offset(env, HDL2VAL(self), n + l - 1, &end)) {
        const char* fmt = "Length %d out of range [0, %d]";
        int_t l = VAL2INT(HDL2VAL(len));
        YogError_raise_ValueError(env, fmt, l, STRING_SIZE(HDL2VAL(self)) - n);
        /* NOTREACHED */
    }
    return YogString_slice(env, self, begin, end - begin + 1);
}

static YogVal
subscript(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* index)
{
    CHECK_SELF_TYPE2(env, self);
    return YogString_subscript(env, HDL2VAL(self), HDL2VAL(index));
}

static YogVal
assign_subscript(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal index = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, index, val);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = {
        { "index", &index },
        { "value", &val },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]=", params, args, kw);
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "string index must be Fixnum");
    }
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "value must be String");
    }

    uint_t offset = unnormalized_index2offset(env, self, VAL2INT(index));
    STRING_CHARS(self)[offset] = STRING_CHARS(val)[0];

    RETURN(env, val);
}

YogVal
YogString_match(YogEnv* env, YogVal self, YogVal regexp, int_t pos)
{
    int_t n = normalize_index(env, self, pos);
    uint_t offset = 0;
    uint_t size = YogString_size(env, self);
    if ((0 < size) && !index2offset(env, self, n, &offset)) {
        return YNIL;
    }

    OnigUChar* str = (OnigUChar*)STRING_CHARS(self);
    OnigUChar* end = (OnigUChar*)&STRING_CHARS(self)[STRING_SIZE(self)];
    OnigUChar* start = (OnigUChar*)&STRING_CHARS(self)[offset];
    OnigRegion* region = onig_region_new();
    int_t r = onig_search(PTR_AS(YogRegexp, regexp)->onig_regexp, str, end, start, end, region, ONIG_OPTION_NONE);
    if (r == ONIG_MISMATCH) {
        return YNIL;
    }

    return YogMatch_new(env, self, regexp, region);
}

YogVal
YogString_binop_match(YogEnv* env, YogHandle* self, YogHandle* regexp)
{
    YogVal re = HDL2VAL(regexp);
    if (!IS_PTR(re) || (BASIC_OBJ_TYPE(re) != TYPE_REGEXP)) {
        const char* fmt = "Can't convert %C object to Regexp implicitly";
        YogError_raise_TypeError(env, fmt, re);
        /* NOTREACHED */
    }
    return YogString_match(env, HDL2VAL(self), re, 0);
}

static YogVal
match(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* regexp)
{
    return YogString_binop_match(env, self, regexp);
}

int_t
YogString_strrchr(YogEnv* env, YogVal self, char c)
{
    uint_t size = STRING_SIZE(self);
    uint_t i;
    for (i = size; 0 < i; i--) {
        if (STRING_CHARS(self)[i - 1] == c) {
            return i - 1;
        }
    }
    return -1;
}

int_t
YogString_find_char(YogEnv* env, YogVal self, uint_t start, YogChar c)
{
    uint_t size = STRING_SIZE(self);
    uint_t i;
    for (i = start; i < size; i++) {
        if (STRING_CHARS(self)[i] == c) {
            return i;
        }
    }
    return -1;
}

static YogVal
each_line(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal arg[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(arg), arg);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "each_line", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    YogHandle* h = YogHandle_REGISTER(env, self);
    uint_t i = 0;
    uint_t size = YogString_size(env, self);
    while (i < size) {
        int_t pos = YogString_find_char(env, self, i, '\n');
        uint_t len = pos < 0 ? STRING_SIZE(self) : pos - i + 1;
        arg[0] = YogString_slice(env, h, i, len);
        YogCallable_call(env, block, array_sizeof(arg), arg);
        i++;
    }

    RETURN(env, YNIL);
}

static YogVal
dump(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "dump", params, args, kw);

    s = YogString_new(env);
#define FORMAT              "0x%08x"
#define ADD_CHAR(fmt, i)    do { \
    char buf[11]; \
    YogSysdeps_snprintf(buf, array_sizeof(buf), fmt, STRING_CHARS(self)[i]); \
    YogString_append_string(env, s, buf); \
} while (0)
    ADD_CHAR(FORMAT, 0);
    uint_t size = STRING_SIZE(self);
    uint_t i;
    for (i = 1; i < size; i++) {
        ADD_CHAR(" " FORMAT, i);
    }
#undef ADD_CHAR
#undef FORMAT

    RETURN(env, s);
}

static YogVal
each_char(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal enc = YUNDEF;
    PUSH_LOCAL(env, enc);
    YogVal a[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(a), a);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "each_char", params, args, kw);

    uint_t size = STRING_SIZE(self);
    uint_t i;
    for (i = 0; i < size; i++) {
        a[0] = get_at(env, self, i);
        YogCallable_call(env, block, array_sizeof(a), a);
    }

    RETURN(env, self);
}

int_t
YogString_hash(YogEnv* env, YogVal self)
{
    int_t val = 0;
    uint_t size = STRING_SIZE(self);
    uint_t i;
    for (i = 0; i < size; i++) {
        val = val * 997 + STRING_CHARS(self)[i];
    }

    return val + (val >> 5);
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "hash", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    int_t hash = YogString_hash(env, self);

    RETURN(env, INT2VAL(hash));
}

static void
normalize_as_number(YogEnv* env, YogVal self, YogVal* normalized, int_t* base)
{
    YOG_ASSERT(env, normalized != NULL, "normalized is NULL");
    YOG_ASSERT(env, base != NULL, "base is NULL");
    SAVE_ARG(env, self);
    YogVal body = YUNDEF;
    PUSH_LOCAL(env, body);

    uint_t size = STRING_SIZE(self);
    if (size == 0) {
        YogError_raise_ValueError(env, "Empty string to convert into integer");
        /* NOTREACHED */
    }
    *normalized = YogString_new(env);

    body = PTR_AS(YogString, self)->body;
    uint_t next_index = 0;
#define NEXTC PTR_AS(YogCharArray, body)->items[next_index]
    YogChar c = NEXTC;
    if (c == '+') {
        next_index++;
    }
    else if (c == '-') {
        YogString_push(env, *normalized, c);
        next_index++;
    }
    if (size <= next_index) {
        YogError_raise_ValueError(env, "String contains only sign: %S", self);
        /* NOTREACHED */
    }

    YogChar c1 = NEXTC;
    if ((c1 == '0') && (next_index < size - 1)) {
        YogChar c2 = PTR_AS(YogCharArray, body)->items[next_index + 1];
        if ((c2 == 'b') || (c2 == 'B')) {
            *base = 2;
            next_index += 2;
        }
        else if ((c2 == 'o') || (c2 == 'O')) {
            *base = 8;
            next_index += 2;
        }
        else if ((c2 == 'd') || (c2 == 'D')) {
            *base = 10;
            next_index += 2;
        }
        else if ((c2 == 'x') || (c2 == 'X')) {
            *base = 16;
            next_index += 2;
        }
        else {
            YogString_push(env, *normalized, c1);
            *base = 10;
            next_index += 1;
        }
        if (size <= next_index) {
            YogError_raise_ValueError(env, "String ends with radix: %S", self);
            /* NOTREACHED */
        }
    }
    else {
        YogString_push(env, *normalized, c1);
        *base = 10;
        next_index += 1;
    }

    while (next_index < size) {
        c = NEXTC;
        if (isalpha(c)) {
            YogString_push(env, *normalized, tolower(c));
            next_index++;
        }
        else if (c == '_') {
            next_index++;
            if (size <= next_index) {
                const char* fmt = "String ends with \"_\": %S";
                YogError_raise_ValueError(env, fmt, self);
                /* NOTREACHED */
            }
            c = NEXTC;
            if (!isalpha(c) && !isdigit(c)) {
                const char* fmt = "No alphabet or digit follows \"_\": %S";
                YogError_raise_ValueError(env, fmt, self);
                /* NOTREACHED */
            }
        }
        else {
            YogString_push(env, *normalized, c);
            next_index++;
        }
    }
#undef NEXTC

    RETURN_VOID(env);
}

YogVal
YogString_to_i(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal normalized = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal bin = YUNDEF;
    PUSH_LOCALS3(env, normalized, bignum, bin);

#define RAISE_VALUE_ERROR   do { \
    YogError_raise_ValueError(env, "Invalid literal: %S", self); \
    RETURN(env, INT2VAL(0)); \
} while (0)
    int_t base;
    normalize_as_number(env, self, &normalized, &base);

    YogHandle* h_enc = YogHandle_REGISTER(env, env->vm->encAscii);
    YogHandle* h_normalized = YogHandle_REGISTER(env, normalized);
    bin = YogEncoding_conv_from_yog(env, h_enc, h_normalized);
    char* endptr = NULL;
    errno = 0;
    long n = strtol(BINARY_CSTR(bin), &endptr, base);
    if (*endptr != '\0') {
        RAISE_VALUE_ERROR;
    }
    if (errno == 0) {
        YogVal v = YogVal_from_int(env, n);
        RETURN(env, v);
    }
    else if (errno == ERANGE) {
        bignum = YogBignum_from_str(env, normalized, base);
        RETURN(env, bignum);
    }

    RAISE_VALUE_ERROR;
#undef RAISE_VALUE_ERROR

    /* NOTREACHED */
    RETURN(env, self);
}

void
YogString_append_string(YogEnv* env, YogVal self, const char* s)
{
    SAVE_ARG(env, self);
    YogVal body = YUNDEF;
    PUSH_LOCAL(env, body);

    YogHandle* h_s = YogHandle_REGISTER(env, YogString_from_string(env, s));
    uint_t size1 = STRING_SIZE(self);
    uint_t size2 = STRING_SIZE(HDL2VAL(h_s));
    uint_t size = size1 + size2;
    YOG_ASSERT(env, size1 <= size, "maybe overflow (%u + %u = %u)", size1, size2, size);
    body = YogCharArray_new(env, size);
    YogChar* top = PTR_AS(YogCharArray, body)->items;
    if (0 < size1) {
        memcpy(top, STRING_CHARS(self), sizeof(YogChar) * size1);
    }
    memcpy(top + size1, STRING_CHARS(HDL2VAL(h_s)), sizeof(YogChar) * size2);
    YogGC_UPDATE_PTR(env, PTR_AS(YogString, self), body, body);
    STRING_SIZE(self) = size;

    RETURN_VOID(env);
}

YogVal
YogString_binop_ufo(YogEnv* env, YogVal self, YogVal s)
{
    if (!IS_PTR(s) || (BASIC_OBJ_TYPE(s) != TYPE_STRING)) {
        return YNIL;
    }

    if (STRING_SIZE(self) < STRING_SIZE(s)) {
        return INT2VAL(-1);
    }
    if (STRING_SIZE(s) < STRING_SIZE(self)) {
        return INT2VAL(1);
    }
    uint_t n = sizeof(YogChar) * STRING_SIZE(self);
    int_t m = memcmp(STRING_CHARS(self), STRING_CHARS(s), n);
    if (m < 0) {
        return INT2VAL(-1);
    }
    if (m == 0) {
        return INT2VAL(0);
    }
    return INT2VAL(1);
}

static YogVal
ufo(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogString_binop_ufo(env, HDL2VAL(self), HDL2VAL(n));
}

YogVal
YogString_to_bin_in_default_encoding(YogEnv* env, YogHandle* self)
{
    YogHandle* enc = YogHandle_REGISTER(env, env->vm->encUtf8);
    return YogEncoding_conv_from_yog(env, enc, self);
}

void
YogString_eval_builtin_script(YogEnv* env, YogVal klass)
{
#if !defined(MINIYOG)
    const char* src =
#   include "string.inc"
    ;
    YogMisc_eval_source(env, VAL2HDL(env, klass), src);
#endif
}

void
YogString_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cString = YUNDEF;
    PUSH_LOCAL(env, cString);
    YogVM* vm = env->vm;

    cString = YogClass_new(env, "String", vm->cObject);
    YogClass_define_allocator(env, cString, alloc);
    YogClass_include_module(env, cString, vm->mComparable);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cString, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("[]=", assign_subscript);
    DEFINE_METHOD("dump", dump);
    DEFINE_METHOD("each_char", each_char);
    DEFINE_METHOD("each_line", each_line);
    DEFINE_METHOD("get", get);
    DEFINE_METHOD("gsub", gsub);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("to_i", to_i);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...)  do { \
    YogClass_define_method2(env, cString, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("*", multiply, "n", NULL);
    DEFINE_METHOD2("+", add, "s", NULL);
    DEFINE_METHOD2("<<", lshift, "s", NULL);
    DEFINE_METHOD2("<=>", ufo, "n", NULL);
    DEFINE_METHOD2("=~", match, "regexp", NULL);
    DEFINE_METHOD2("[]", subscript, "index", NULL);
    DEFINE_METHOD2("slice", slice, "pos", "|", "len", NULL);
    DEFINE_METHOD2("to_bin", to_bin, "encoding", NULL);
#undef DEFINE_METHOD2
#define DEFINE_PROP(name, getter, setter) do { \
    YogClass_define_property(env, cString, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("size", get_size, NULL);
#undef DEFINE_PROP
    vm->cString = cString;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
