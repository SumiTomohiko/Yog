#include "yog/config.h"
#if defined(YOG_HAVE_STDINT_H)
#   include <stdint.h>
#endif
#include <stdio.h>
#include <string.h>
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_BINARY(env, v, name) do { \
    if (!IS_PTR((v)) || (BASIC_OBJ_TYPE((v)) != TYPE_BINARY)) { \
        YogError_raise_TypeError((env), "%s must be Binary, not %C", (name), (v)); \
    } \
} while (0)
#define CHECK_SELF_BINARY CHECK_BINARY(env, self, "self")

uint_t
YogBinary_size(YogEnv* env, YogVal binary)
{
    return PTR_AS(YogBinary, binary)->size;
}

void
YogBinary_shrink(YogEnv* env, YogVal binary)
{
    SAVE_ARG(env, binary);

    uint_t size = YogBinary_size(env, binary);
    YogVal new_body = YogByteArray_new(env, size);
    char* to = PTR_AS(YogByteArray, new_body)->items;
    YogVal old_body = PTR_AS(YogBinary, binary)->body;
    char* from = PTR_AS(YogByteArray, old_body)->items;
    memcpy(to, from, size);
    YogGC_UPDATE_PTR(env, PTR_AS(YogBinary, binary), body, new_body);

    RETURN_VOID(env);
}

uint_t
YogByteArray_size(YogEnv* env, YogVal array)
{
    return PTR_AS(YogByteArray, array)->size;
}

YogVal
YogByteArray_new(YogEnv* env, uint_t size)
{
    YogGC_check_multiply_overflow(env, size, sizeof(uint8_t));
    YogVal array = ALLOC_OBJ_ITEM(env, NULL, NULL, YogByteArray, size, uint8_t);
    PTR_AS(YogByteArray, array)->size = size;

    return array;
}

static void
ensure_body_size(YogEnv* env, YogVal binary, uint_t needed_size)
{
    SAVE_ARG(env, binary);
    YogVal body = PTR_AS(YogBinary, binary)->body;
    PUSH_LOCAL(env, body);

    if (IS_PTR(body) && (needed_size <= PTR_AS(YogByteArray, body)->size)) {
        RETURN_VOID(env);
    }

    uint_t new_size = 2 * needed_size;
    YogVal new_body = YogByteArray_new(env, new_size);
    char* to = PTR_AS(YogByteArray, new_body)->items;
    if (IS_PTR(body)) {
        char* from = PTR_AS(YogByteArray, body)->items;
        size_t cur_size = PTR_AS(YogBinary, binary)->size;
        memcpy(to, from, cur_size);
    }
    YogGC_UPDATE_PTR(env, PTR_AS(YogBinary, binary), body, new_body);

    RETURN_VOID(env);
}

#define PUSH_TYPE(type, n)  do { \
    SAVE_ARG(env, binary); \
\
    size_t needed_size = PTR_AS(YogBinary, binary)->size + sizeof(type); \
    ensure_body_size(env, binary, needed_size); \
\
    YogVal body = PTR_AS(YogBinary, binary)->body; \
    uint_t size = PTR_AS(YogBinary, binary)->size; \
    *((type*)&PTR_AS(YogByteArray, body)->items[size]) = n; \
    PTR_AS(YogBinary, binary)->size += sizeof(type); \
\
    RETURN_VOID(env); \
} while (0)
void
YogBinary_push_uint8(YogEnv* env, YogVal binary, uint8_t n)
{
    PUSH_TYPE(uint8_t, n);
}

void
YogBinary_push_char(YogEnv* env, YogVal binary, char c)
{
    PUSH_TYPE(char, c);
}

void
YogBinary_push_id(YogEnv* env, YogVal binary, ID id)
{
    PUSH_TYPE(ID, id);
}

void
YogBinary_push_uint(YogEnv* env, YogVal binary, uint_t n)
{
    PUSH_TYPE(uint_t, n);
}

void
YogBinary_push_pc(YogEnv* env, YogVal binary, pc_t pc)
{
    PUSH_TYPE(pc_t, pc);
}
#undef PUSH_TYPE

static void
YogBinary_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBinary* bin = PTR_AS(YogBinary, ptr);
    YogBasicObj_keep_children(env, ptr, keeper, heap);
    YogGC_KEEP(env, bin, body, keeper, heap);
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal bin = YUNDEF;
    PUSH_LOCAL(env, bin);

    bin = ALLOC_OBJ(env, YogBinary_keep_children, NULL, YogBinary);
    YogBasicObj_init(env, bin, TYPE_BINARY, 0, klass);
    PTR_AS(YogBinary, bin)->size = 0;
    PTR_AS(YogBinary, bin)->body = YUNDEF;

    RETURN(env, bin);
}

void
YogBinary_add(YogEnv* env, YogVal self, const char* buf, uint_t size)
{
    SAVE_ARG(env, self);

    uint_t current_size = YogBinary_size(env, self);
    uint_t needed_size = current_size + size;
    ensure_body_size(env, self, needed_size);
    memcpy(&BINARY_CSTR(self)[current_size], buf, size);
    PTR_AS(YogBinary, self)->size = needed_size;

    RETURN_VOID(env);
}

YogVal
YogBinary_of_size(YogEnv* env, uint_t size)
{
    SAVE_LOCALS(env);
    YogVal bin = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS2(env, bin, body);

    bin = alloc(env, env->vm->cBinary);
    body = YogByteArray_new(env, size);
    YogGC_UPDATE_PTR(env, PTR_AS(YogBinary, bin), body, body);

    RETURN(env, bin);
}

YogVal
YogBinary_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal bin = YUNDEF;
    PUSH_LOCAL(env, bin);

    bin = alloc(env, env->vm->cBinary);

    RETURN(env, bin);
}

static YogVal
get_size(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal size = YUNDEF;
    PUSH_LOCAL(env, size);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_size", params, args, kw);
    CHECK_SELF_BINARY;
    size = YogVal_from_unsigned_int(env, BINARY_SIZE(self));
    RETURN(env, size);
}

static void
lshift_int(YogEnv* env, YogVal self, int_t n)
{
    SAVE_ARG(env, self);
    if ((n < 0) || (UCHAR_MAX < n)) {
        const char* fmt = "Value must be between 0 and 255, but %d given";
        YogError_raise_ValueError(env, fmt, n);
        /* NOTREACHED */
    }
    uint_t size = YogBinary_size(env, self);

    ensure_body_size(env, self, size + 1);
    BINARY_CSTR(self)[size] = n;
    BINARY_SIZE(self)++;

    RETURN_VOID(env);
}

static YogVal
lshift(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal bin = YUNDEF;
    PUSH_LOCAL(env, bin);
    YogCArg params[] = { { "bin", &bin }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "<<", params, args, kw);
    CHECK_SELF_BINARY;

    if (IS_FIXNUM(bin)) {
        lshift_int(env, self, VAL2INT(bin));
        RETURN(env, self);
    }

    CHECK_BINARY(env, bin, "bin");

    uint_t size1 = YogBinary_size(env, self);
    uint_t size2 = YogBinary_size(env, bin);
    uint_t size = size1 + size2;
    if ((size < size1) || (size < size2)) {
        YogError_raise_ValueError(env, "Too large Binary");
    }
    ensure_body_size(env, self, size);
    memcpy(BINARY_CSTR(self) + size1, BINARY_CSTR(bin), size2);
    BINARY_SIZE(self) = size;
    RETURN(env, self);
}

static YogVal
to_bin(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_bin", params, args, kw);
    RETURN(env, self);
}

YogVal
YogBinary_to_s(YogEnv* env, YogVal self, YogHandle* encoding)
{
    /* XXX: self must include the terminating NUL character. */
    uint_t size = BINARY_SIZE(self);
    char buf[size];
    memcpy(buf, BINARY_CSTR(self), size);
    return YogEncoding_conv_to_yog(env, encoding, buf, buf + size - 1);
}

static YogVal
YogBinary_slice(YogEnv* env, YogVal self, int_t pos, int_t len)
{
    SAVE_ARG(env, self);
    YogVal bin = YUNDEF;
    PUSH_LOCAL(env, bin);
    uint_t size = YogBinary_size(env, self);
    if (pos < 0) {
        pos += size;
    }
    if (pos < 0) {
        YogError_raise_ValueError(env, "Binary index out of range");
    }
    if (len < 0) {
        YogError_raise_ValueError(env, "Binary length out of range");
    }
    if (size <= pos) {
        RETURN(env, YogBinary_new(env));
    }

    bin = YogBinary_of_size(env, len);
    memcpy(BINARY_CSTR(bin), &BINARY_CSTR(self)[pos], len);
    BINARY_SIZE(bin) = len;
    RETURN(env, bin);
}

static YogVal
slice(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal pos = YUNDEF;
    YogVal len = YNIL;
    PUSH_LOCALS2(env, pos, len);
    YogCArg params[] = {
        { "pos", &pos },
        { "|", NULL },
        { "len", &len },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "slice", params, args, kw);
    CHECK_SELF_BINARY;
    if (!IS_FIXNUM(pos)) {
        YogError_raise_TypeError(env, "pos must be Fixnum, not %C", pos);
    }
    if (!IS_NIL(len) && !IS_FIXNUM(len)) {
        YogError_raise_TypeError(env, "len must be Fixnum, not %C", len);
    }

    if (IS_NIL(len)) {
        uint_t size = YogBinary_size(env, self);
        RETURN(env, YogBinary_slice(env, self, VAL2INT(pos), size));
    }
    RETURN(env, YogBinary_slice(env, self, VAL2INT(pos), VAL2INT(len)));
}

static YogVal
inspect(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "inspect", params, args, kw);
    CHECK_SELF_BINARY;

    if (!IS_PTR(BINARY_BODY(self))) {
        RETURN(env, YogString_from_string(env, "b\"\""));
    }
    s = YogString_from_string(env, "b\"");
    uint_t size = YogBinary_size(env, self);
    uint_t i;
    for (i = 0; i < size; i++) {
        char c = BINARY_CSTR(self)[i];
        char buf[5];
        YogSysdeps_snprintf(buf, array_sizeof(buf), "\\x%02x", 0xff & c);
        YogString_append_string(env, s, buf);
    }
    YogString_append_string(env, s, "\"");

    RETURN(env, s);
}

static YogVal
subscript(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* index)
{
    YogMisc_check_Fixnum(env, index, "index");
    int_t n = VAL2INT(HDL2VAL(index));
    uint_t size = BINARY_SIZE(HDL2VAL(self));
    int_t pos = n + (n < 0 ? size : 0);
    if ((pos < 0) || (size <= pos)) {
        const char* fmt = "Binary index out of range: size is %u, but %d given";
        YogError_raise_IndexError(env, fmt, size, n);
        /* NOTREACHED */
    }

    return INT2VAL((unsigned char)BINARY_CSTR(HDL2VAL(self))[pos]);
}

static YogVal
to_s(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* encoding)
{
    YogMisc_check_Encoding(env, encoding, "encoding");
    return YogBinary_to_s(env, HDL2VAL(self), encoding);
}

void
YogBinary_eval_builtin_script(YogEnv* env, YogVal klass)
{
    const char* src =
#include "binary.inc"
    ;
    YogMisc_eval_source(env, VAL2HDL(env, klass), src);
}

void
YogBinary_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cBinary = YUNDEF;
    PUSH_LOCAL(env, cBinary);

    YogVM* vm = env->vm;
    cBinary = YogClass_new(env, "Binary", vm->cObject);
    YogClass_define_allocator(env, cBinary, alloc);
#define DEFINE_METHOD(name, f) do { \
    YogClass_define_method(env, cBinary, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("<<", lshift);
    DEFINE_METHOD("inspect", inspect);
    DEFINE_METHOD("slice", slice);
    DEFINE_METHOD("to_bin", to_bin);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...)  do { \
    YogClass_define_method2(env, cBinary, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("[]", subscript, "index", NULL);
    DEFINE_METHOD2("to_s", to_s, "encoding", NULL);
#undef DEFINE_METHOD2
    YogClass_define_property(env, cBinary, pkg, "size", get_size, NULL);
    vm->cBinary = cBinary;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
