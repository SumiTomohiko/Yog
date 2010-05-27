#include "yog/config.h"
#if defined(HAVE_STDLIB_H)
#   include <stdlib.h>
#endif
#if defined(HAVE_STRING_H)
#   include <string.h>
#endif
#if defined(HAVE_STRINGS_H)
#   include <strings.h>
#endif
#include "ffi.h"
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/float.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Lib {
    struct YogBasicObj base;
    LIB_HANDLE handle;
};

typedef struct Lib Lib;

#define TYPE_LIB TO_TYPE(Lib_alloc)

struct LibFunc {
    struct YogBasicObj base;
    ffi_cif cif;
    void* f;
    uint_t nargs;
    YogVal arg_types[0];
};

typedef struct LibFunc LibFunc;

#define TYPE_LIB_FUNC TO_TYPE(LibFunc_new)

struct Field {
    struct YogBasicObj base;
    ID name;
    YogVal type;
    uint_t offset;
};

typedef struct Field Field;

#define TYPE_FIELD TO_TYPE(Field_alloc)

struct StructClass {
    struct YogClass base;
    uint_t size;
};

typedef struct StructClass StructClass;

#define TYPE_STRUCT_CLASS TO_TYPE(StructClassClass_new)
static YogVal StructClassClass_new(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block);

struct Struct {
    struct YogBasicObj base;
    void* data;
};

typedef struct Struct Struct;

#define TYPE_STRUCT TO_TYPE(Struct_alloc)

static YogVal Struct_alloc(YogEnv* env, YogVal klass);

static void
Field_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    Field* field = (Field*)ptr;
    YogGC_KEEP(env, field, type, keeper, heap);
}

static YogVal
Field_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = ALLOC_OBJ(env, Field_keep_children, NULL, Field);
    YogBasicObj_init(env, obj, TYPE_FIELD, 0, klass);
    PTR_AS(Field, obj)->name = INVALID_ID;
    PTR_AS(Field, obj)->type = YUNDEF;
    PTR_AS(Field, obj)->offset = 0;

    RETURN(env, obj);
}

static YogVal
Field_new(YogEnv* env, ID name, YogVal type, uint_t offset)
{
    SAVE_ARG(env, type);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    if (!IS_SYMBOL(type) && (!IS_PTR(type) || (type != env->vm->cString))) {
        YogError_raise_TypeError(env, "Type must be Symbol or String, not %C", type);
    }

    field = Field_alloc(env, env->vm->cField);
    PTR_AS(Field, field)->name = name;
    PTR_AS(Field, field)->type = type;
    PTR_AS(Field, field)->offset = offset;

    RETURN(env, field);
}

static void
StructClass_set_field(YogEnv* env, YogVal self, uint_t index, YogVal name, YogVal type, uint_t offset)
{
    SAVE_ARGS3(env, self, name, type);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    if (!IS_SYMBOL(name)) {
        YogError_raise_TypeError(env, "Name must be Symbol, not %C", name);
    }
    field = Field_new(env, VAL2ID(name), type, offset);
    YogObj_set_attr_id(env, self, VAL2ID(name), field);

    RETURN_VOID(env);
}

static void
StructClass_init(YogEnv* env, YogVal self, uint_t fields_num)
{
    SAVE_ARG(env, self);

    YogClass_init(env, self, TYPE_STRUCT_CLASS, env->vm->cStructClass);
    PTR_AS(StructClass, self)->size = 0;
    YogGC_UPDATE_PTR(env, PTR_AS(YogClass, self), super, env->vm->cClass);

    RETURN_VOID(env);
}

static uint_t
id2size(YogEnv* env, ID type)
{
    SAVE_LOCALS(env);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogVM_id2name(env, env->vm, type);
    const char* t = STRING_CSTR(s);
    uint_t size;
    if (strcmp(t, "uint8") == 0) {
        size = sizeof(uint8_t);
    }
    else if (strcmp(t, "int8") == 0) {
        size = sizeof(int8_t);
    }
    else if (strcmp(t, "uint16") == 0) {
        size = sizeof(uint16_t);
    }
    else if (strcmp(t, "int16") == 0) {
        size = sizeof(int16_t);
    }
    else if (strcmp(t, "uint32") == 0) {
        size = sizeof(uint32_t);
    }
    else if (strcmp(t, "int32") == 0) {
        size = sizeof(int32_t);
    }
    else if (strcmp(t, "uint64") == 0) {
        size = sizeof(uint64_t);
    }
    else if (strcmp(t, "int64") == 0) {
        size = sizeof(int64_t);
    }
    else if (strcmp(t, "float") == 0) {
        size = sizeof(float);
    }
    else if (strcmp(t, "double") == 0) {
        size = sizeof(double);
    }
    else if (strcmp(t, "uchar") == 0) {
        size = sizeof(unsigned char);
    }
    else if (strcmp(t, "char") == 0) {
        size = sizeof(char);
    }
    else if (strcmp(t, "ushort") == 0) {
        size = sizeof(unsigned short);
    }
    else if (strcmp(t, "short") == 0) {
        size = sizeof(short);
    }
    else if (strcmp(t, "uint") == 0) {
        size = sizeof(unsigned int);
    }
    else if (strcmp(t, "int") == 0) {
        size = sizeof(int);
    }
    else if (strcmp(t, "ulong") == 0) {
        size = sizeof(unsigned long);
    }
    else if (strcmp(t, "long") == 0) {
        size = sizeof(long);
    }
    else if (strcmp(t, "longdouble") == 0) {
        size = sizeof(long double);
    }
    else if (strcmp(t, "pointer") == 0) {
        size = sizeof(void*);
    }
    else {
        YogError_raise_ValueError(env, "unknown type - %S", s);
        /* NOTREACHED */
    }

    RETURN(env, size);
}

static uint_t
type2size(YogEnv* env, ffi_type* type)
{
    SAVE_LOCALS(env);

    uint_t size;
    if (type == &ffi_type_uint8) {
        size = sizeof(uint8_t);
    }
    else if (type == &ffi_type_sint8) {
        size = sizeof(int8_t);
    }
    else if (type == &ffi_type_uint16) {
        size = sizeof(uint16_t);
    }
    else if (type == &ffi_type_sint16) {
        size = sizeof(int16_t);
    }
    else if (type == &ffi_type_uint32) {
        size = sizeof(uint32_t);
    }
    else if (type == &ffi_type_sint32) {
        size = sizeof(int32_t);
    }
    else if (type == &ffi_type_uint64) {
        size = sizeof(uint64_t);
    }
    else if (type == &ffi_type_sint64) {
        size = sizeof(int64_t);
    }
    else if (type == &ffi_type_float) {
        size = sizeof(float);
    }
    else if (type == &ffi_type_double) {
        size = sizeof(double);
    }
    else if (type == &ffi_type_uchar) {
        size = sizeof(unsigned char);
    }
    else if (type == &ffi_type_schar) {
        size = sizeof(char);
    }
    else if (type == &ffi_type_ushort) {
        size = sizeof(unsigned short);
    }
    else if (type == &ffi_type_sshort) {
        size = sizeof(short);
    }
    else if (type == &ffi_type_uint) {
        size = sizeof(unsigned int);
    }
    else if (type == &ffi_type_sint) {
        size = sizeof(int);
    }
    else if (type == &ffi_type_ulong) {
        size = sizeof(unsigned long);
    }
    else if (type == &ffi_type_slong) {
        size = sizeof(long);
    }
    else if (type == &ffi_type_longdouble) {
        size = sizeof(long double);
    }
    else if (type == &ffi_type_pointer) {
        size = sizeof(void*);
    }
    else {
        YogError_raise_ValueError(env, "Unknown FFI type");
        /* NOTREACHED */
    }

    RETURN(env, size);
}

#define ALIGN(offset, alignment) \
            (((offset) + (alignment) - 1) & ~((alignment) - 1))
#define SIZEOF(env, val) \
            (IS_SYMBOL((val)) ? id2size((env), VAL2ID((val))) : sizeof(void*))

static uint_t
align_offset(YogEnv* env, YogVal type, uint_t offset)
{
    SAVE_ARG(env, type);
    uint_t size = SIZEOF(env, type);
    uint_t alignment = sizeof(void*) < size ? sizeof(void*) : size;
    RETURN(env, ALIGN(offset, alignment));
}

static YogVal
StructClassClass_new(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    YogVal fields = YUNDEF;
    YogVal field = YUNDEF;
    YogVal name = YUNDEF;
    YogVal field_type = YUNDEF;
    YogVal field_name = YUNDEF;
    PUSH_LOCALS6(env, name, obj, fields, field, field_type, field_name);
    YogCArg params[] = {
        { "name", &name },
        { "fields", &fields },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "new", params, args, kw);
    if (!IS_PTR(name) || (BASIC_OBJ_TYPE(name) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "name must be String, not %C", name);
    }
    if (!IS_PTR(fields) || (BASIC_OBJ_TYPE(fields) != TYPE_ARRAY)) {
        YogError_raise_TypeError(env, "fields must be Array, not %C", fields);
    }

    uint_t fields_num = YogArray_size(env, fields);
    obj = ALLOC_OBJ(env, YogClass_keep_children, NULL, StructClass);
    StructClass_init(env, obj, fields_num);

    uint_t size = YogString_size(env, name) + 1; /* with '\0' */
    char* s = (char*)YogSysdeps_alloca(sizeof(char) * size);
    memcpy(s, STRING_CSTR(name), size);
    ID id = YogVM_intern(env, env->vm, s);
    PTR_AS(YogClass, obj)->name = id;
    YogClass_define_allocator(env, obj, Struct_alloc);

    uint_t offset = 0;
    uint_t i;
    for (i = 0; i < fields_num; i++) {
        field = YogArray_at(env, fields, i);
        field_type = YogArray_at(env, field, 0);
        offset = align_offset(env, field_type, offset);
        field_name = YogArray_at(env, field, 1);
        StructClass_set_field(env, obj, i, field_name, field_type, offset);
        offset += SIZEOF(env, field_type);
    }
    PTR_AS(StructClass, obj)->size = ALIGN(offset, sizeof(void*));

    RETURN(env, obj);
}

static YogVal
Struct_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(klass) == TYPE_STRUCT_CLASS, "invalid class");

    obj = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Struct);
    YogBasicObj_init(env, obj, TYPE_STRUCT, 0, klass);
    uint_t size = PTR_AS(StructClass, klass)->size;
    PTR_AS(Struct, obj)->data = YogGC_malloc(env, size);
    bzero(PTR_AS(Struct, obj)->data, size);

    RETURN(env, obj);
}

static void
LibFunc_finalize(YogEnv* env, void* ptr)
{
    LibFunc* f = (LibFunc*)ptr;
    free(f->cif.arg_types);
}

static void
LibFunc_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    LibFunc* f = (LibFunc*)ptr;
    uint_t nargs = f->nargs;
    uint_t i;
    for (i = 0; i < nargs; i++) {
        YogGC_KEEP(env, f, arg_types[i], keeper, heap);
    }
}

static YogVal
LibFunc_new(YogEnv* env, uint_t nargs)
{
    SAVE_LOCALS(env);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = ALLOC_OBJ_ITEM(env, LibFunc_keep_children, LibFunc_finalize, LibFunc, nargs, YogVal);
    YogBasicObj_init(env, obj, TYPE_LIB_FUNC, 0, env->vm->cLibFunc);
    PTR_AS(LibFunc, obj)->f = NULL;
    PTR_AS(LibFunc, obj)->cif.arg_types = NULL;
    PTR_AS(LibFunc, obj)->nargs = nargs;
    uint_t i;
    for (i = 0; i < nargs; i++) {
        PTR_AS(LibFunc, obj)->arg_types[i] = YUNDEF;
    }

    RETURN(env, obj);
}

static YogVal
Lib_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Lib);
    YogBasicObj_init(env, obj, TYPE_LIB, 0, klass);
    PTR_AS(Lib, obj)->handle = NULL;

    RETURN(env, obj);
}

YogVal
YogFFI_load_lib(YogEnv* env, YogVal path)
{
    SAVE_ARG(env, path);
    YogVal lib = YUNDEF;
    PUSH_LOCAL(env, lib);

    LIB_HANDLE handle = YogSysdeps_open_lib(STRING_CSTR(path));
    if (handle == NULL) {
        YogError_raise_ImportError(env, "no library named \"%S\"", path);
    }
    lib = Lib_alloc(env, env->vm->cLib);
    PTR_AS(Lib, lib)->handle = handle;

    RETURN(env, lib);
}

static ffi_type*
map_id_type(YogEnv* env, ID type)
{
    SAVE_LOCALS(env);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogVM_id2name(env, env->vm, type);
    const char* t = STRING_CSTR(s);
    ffi_type* cif_type;
    if (strcmp(t, "void") == 0) {
        cif_type = &ffi_type_void;
    }
    else if (strcmp(t, "uint8") == 0) {
        cif_type = &ffi_type_uint8;
    }
    else if (strcmp(t, "int8") == 0) {
        cif_type = &ffi_type_sint8;
    }
    else if (strcmp(t, "uint16") == 0) {
        cif_type = &ffi_type_uint16;
    }
    else if (strcmp(t, "int16") == 0) {
        cif_type = &ffi_type_sint16;
    }
    else if (strcmp(t, "uint32") == 0) {
        cif_type = &ffi_type_uint32;
    }
    else if (strcmp(t, "int32") == 0) {
        cif_type = &ffi_type_sint32;
    }
    else if (strcmp(t, "uint64") == 0) {
        cif_type = &ffi_type_uint64;
    }
    else if (strcmp(t, "int64") == 0) {
        cif_type = &ffi_type_sint64;
    }
    else if (strcmp(t, "float") == 0) {
        cif_type = &ffi_type_float;
    }
    else if (strcmp(t, "double") == 0) {
        cif_type = &ffi_type_double;
    }
    else if (strcmp(t, "uchar") == 0) {
        cif_type = &ffi_type_uchar;
    }
    else if (strcmp(t, "char") == 0) {
        cif_type = &ffi_type_schar;
    }
    else if (strcmp(t, "ushort") == 0) {
        cif_type = &ffi_type_ushort;
    }
    else if (strcmp(t, "short") == 0) {
        cif_type = &ffi_type_sshort;
    }
    else if (strcmp(t, "uint") == 0) {
        cif_type = &ffi_type_uint;
    }
    else if (strcmp(t, "int") == 0) {
        cif_type = &ffi_type_sint;
    }
    else if (strcmp(t, "ulong") == 0) {
        cif_type = &ffi_type_ulong;
    }
    else if (strcmp(t, "long") == 0) {
        cif_type = &ffi_type_slong;
    }
    else if (strcmp(t, "longdouble") == 0) {
        cif_type = &ffi_type_longdouble;
    }
    else if (strcmp(t, "pointer") == 0) {
        cif_type = &ffi_type_pointer;
    }
    else {
        YogError_raise_ValueError(env, "unknown type - %S", s);
        /* NOTREACHED */
    }

    RETURN(env, cif_type);
}

static ffi_type*
map_type(YogEnv* env, YogVal type)
{
    SAVE_ARG(env, type);

    if (IS_SYMBOL(type)) {
        RETURN(env, map_id_type(env, VAL2ID(type)));
    }
    if (IS_PTR(type) && (BASIC_OBJ_TYPE(type) == TYPE_STRUCT_CLASS)) {
        RETURN(env, &ffi_type_pointer);
    }
    YogError_raise_TypeError(env, "Type must be StructClass or Symbol, not %C", type);
    /* NOTREACHED */
    RETURN(env, NULL);
}

static const char*
map_ffi_error(YogEnv* env, ffi_status status)
{
    switch (status) {
    case FFI_BAD_TYPEDEF:
        return "One of types is incorrect";
        break;
    case FFI_BAD_ABI:
        return "ABI is invalid";
        break;
    default:
        return "Unknown error";
        break;
    }
}

static YogVal
load_func(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal f = YUNDEF;
    YogVal name = YUNDEF;
    YogVal arg_types = YNIL;
    YogVal rtype = YNIL;
    YogVal arg_type = YUNDEF;
    PUSH_LOCALS5(env, f, name, arg_types, rtype, arg_type);
    YogCArg params[] = {
        { "name", &name },
        { "|", NULL },
        { "arg_types", &arg_types },
        { "rtype", &rtype },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "load_func", params, args, kw);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_LIB)) {
        YogError_raise_TypeError(env, "self must be Lib, not %C", self);
    }
    if (!IS_PTR(name) || (BASIC_OBJ_TYPE(name) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "name must be String, not %C", name);
    }
    if (!IS_NIL(arg_types) && (BASIC_OBJ_TYPE(arg_types) != TYPE_ARRAY)) {
        YogError_raise_TypeError(env, "arg_types must be Array or nil, not %C", arg_types);
    }
    if (!IS_NIL(rtype) && !IS_SYMBOL(rtype)) {
        YogError_raise_TypeError(env, "rtype must be Symbol or nil, not %C", rtype);
    }

    uint_t nargs = IS_NIL(arg_types) ? 0 : YogArray_size(env, arg_types);
    f = LibFunc_new(env, nargs);
    ffi_type** types = (ffi_type**)YogGC_malloc(env, sizeof(ffi_type*) * nargs);
    uint_t i;
    for (i = 0; i < nargs; i++) {
        arg_type = YogArray_at(env, arg_types, i);
        types[i] = map_type(env, arg_type);
        PTR_AS(LibFunc, f)->arg_types[i] = arg_type;
    }
    ffi_status status = ffi_prep_cif(&PTR_AS(LibFunc, f)->cif, FFI_DEFAULT_ABI, nargs, IS_NIL(rtype) ? &ffi_type_void : map_type(env, rtype), types);
    if (status != FFI_OK) {
        const char* s = map_ffi_error(env, status);
        YogError_raise_FFIError(env, "%s", s);
    }
    void* p = YogSysdeps_get_proc(PTR_AS(Lib, self)->handle, STRING_CSTR(name));
    if (p == NULL) {
        YogError_raise_FFIError(env, "Can't find address of %S", name);
    }
    PTR_AS(LibFunc, f)->f = p;

    RETURN(env, f);
}

static void
check_Fixnum(YogEnv* env, YogVal val, int_t min, int_t max)
{
    SAVE_ARG(env, val);

    if (!IS_FIXNUM(val)) {
        YogError_raise_TypeError(env, "Value must be Fixnum, not %C", val);
    }
    if (VAL2INT(val) < min) {
        YogError_raise_ValueError(env, "Value must be greater or equal %d, not %D", min, val);
    }
    if (max < VAL2INT(val)) {
        YogError_raise_ValueError(env, "Value must be less or equal %d, not %D", max, val);
    }

    RETURN_VOID(env);
}

static void
check_Fixnum_uint8(YogEnv* env, YogVal val)
{
    check_Fixnum(env, val, 0, UINT8_MAX);
}

static void
check_Fixnum_int8(YogEnv* env, YogVal val)
{
    check_Fixnum(env, val, INT8_MIN, INT8_MAX);
}

static void
check_Fixnum_uint16(YogEnv* env, YogVal val)
{
    check_Fixnum(env, val, 0, UINT16_MAX);
}

static void
check_Fixnum_int16(YogEnv* env, YogVal val)
{
    check_Fixnum(env, val, INT16_MIN, INT16_MAX);
}

static void
check_Bignum_is_greater_or_equal_than_int(YogEnv* env, YogVal bignum, int_t n)
{
    SAVE_ARG(env, bignum);

    if (0 <= YogBignum_compare_with_int(env, bignum, n)) {
        RETURN_VOID(env);
    }
    YogError_raise_ValueError(env, "Value must be greater or equal %d, not %D", n, bignum);

    RETURN_VOID(env);
}

static void
check_Bignum_is_less_or_equal_than_unsigned_int(YogEnv* env, YogVal bignum, uint_t n)
{
    SAVE_ARG(env, bignum);

    if (YogBignum_compare_with_unsigned_int(env, bignum, n) <= 0) {
        RETURN_VOID(env);
    }
    YogError_raise_ValueError(env, "Value must be less or equal %u, not %D", n, bignum);

    RETURN_VOID(env);
}

#define WRITE_POSITIVE_ARGUMENT(env, type, dest, val) do { \
    if (VAL2INT((val)) < 0) { \
        YogError_raise_ValueError((env), "Value must be greater or equal 0, not %d", VAL2INT((val))); \
    } \
    *((type*)dest) = VAL2INT((val)); \
} while (0)

static void
check_Bignum_uint32(YogEnv* env, YogVal val)
{
    SAVE_ARG(env, val);
    check_Bignum_is_greater_or_equal_than_int(env, val, 0);
    check_Bignum_is_less_or_equal_than_unsigned_int(env, val, UINT32_MAX);
    RETURN_VOID(env);
}

static void
write_argument_uint32(YogEnv* env, uint32_t* dest, YogVal val)
{
    SAVE_ARG(env, val);

    if (IS_FIXNUM(val)) {
        WRITE_POSITIVE_ARGUMENT(env, uint32_t, dest, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_uint32(env, val);
        *dest = YogBignum_to_unsigned_type(env, val, "Value");
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
check_Bignum_is_less_or_equal_than_int(YogEnv* env, YogVal val, int_t n)
{
    SAVE_ARG(env, val);

    if (YogBignum_compare_with_int(env, val, n) <= 0) {
        RETURN_VOID(env);
    }
    YogError_raise_ValueError(env, "Value must be less or equal %d, not %D", n, val);

    RETURN_VOID(env);
}

static void
check_Bignum_int32(YogEnv* env, YogVal val)
{
    SAVE_ARG(env, val);
    check_Bignum_is_greater_or_equal_than_int(env, val, INT32_MIN);
    check_Bignum_is_less_or_equal_than_int(env, val, INT32_MAX);
    RETURN_VOID(env);
}

static void
write_argument_int32(YogEnv* env, int32_t* dest, YogVal val)
{
    SAVE_ARG(env, val);

    if (IS_FIXNUM(val)) {
        *dest = VAL2INT(val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_int32(env, val);
        *dest = YogBignum_to_signed_type(env, val, "Value");
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
check_Bignum_is_less_or_equal_than_unsigned_long_long(YogEnv* env, YogVal bignum, unsigned long long n)
{
    SAVE_ARG(env, bignum);

    if (YogBignum_compare_with_unsigned_long_long(env, bignum, n) <= 0) {
        RETURN_VOID(env);
    }
    YogError_raise_ValueError(env, "Value must be less or equal %llu, not %D", n, bignum);

    RETURN_VOID(env);
}

static void
check_Bignum_uint64(YogEnv* env, YogVal val)
{
    SAVE_ARG(env, val);
    check_Bignum_is_greater_or_equal_than_int(env, val, 0);
    check_Bignum_is_less_or_equal_than_unsigned_long_long(env, val, UINT64_MAX);
    RETURN_VOID(env);
}

static void
write_argument_uint64(YogEnv* env, uint64_t* dest, YogVal val)
{
    SAVE_ARG(env, val);

    if (IS_FIXNUM(val)) {
        WRITE_POSITIVE_ARGUMENT(env, uint64_t, dest, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_uint64(env, val);
        *dest = YogBignum_to_unsigned_long_long(env, val, "Value");
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
check_Bignum_is_greater_or_equal_than_long_long(YogEnv* env, YogVal bignum, long long n)
{
    SAVE_ARG(env, bignum);

    if (0 <= YogBignum_compare_with_long_long(env, bignum, n)) {
        RETURN_VOID(env);
    }
    YogError_raise_ValueError(env, "Value must be greater or equal %lld, not %D", n, bignum);

    RETURN_VOID(env);
}

static void
check_Bignum_is_less_or_equal_than_long_long(YogEnv* env, YogVal bignum, long long n)
{
    SAVE_ARG(env, bignum);

    if (YogBignum_compare_with_long_long(env, bignum, INT64_MAX) <= 0) {
        RETURN_VOID(env);
    }
    YogError_raise_ValueError(env, "Value must be less or equal %llu, not %D", n, bignum);

    RETURN_VOID(env);
}

static void
check_Bignum_int64(YogEnv* env, YogVal val)
{
    SAVE_ARG(env, val);
    check_Bignum_is_greater_or_equal_than_long_long(env, val, INT64_MIN);
    check_Bignum_is_less_or_equal_than_long_long(env, val, INT64_MAX);
    RETURN_VOID(env);
}

static void
write_argument_int64(YogEnv* env, int64_t* dest, YogVal val)
{
    SAVE_ARG(env, val);

    if (IS_FIXNUM(val)) {
        *dest = VAL2INT(val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_int64(env, val);
        *dest = YogBignum_to_long_long(env, val, "Value");
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
write_argument_double(YogEnv* env, double* dest, YogVal val)
{
    SAVE_ARG(env, val);

    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_FLOAT)) {
        *dest = FLOAT_NUM(val);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Float, not %C", val);

    RETURN_VOID(env);
}

static void
write_argument_long_double(YogEnv* env, long double* dest, YogVal val)
{
    SAVE_ARG(env, val);

    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_FLOAT)) {
        *dest = FLOAT_NUM(val);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Float, not %C", val);

    RETURN_VOID(env);
}

static void
write_argument_float(YogEnv* env, float* dest, YogVal val)
{
    SAVE_ARG(env, val);

    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_FLOAT)) {
        *dest = FLOAT_NUM(val);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Float, not %C", val);

    RETURN_VOID(env);
}

static void
write_argument_int8(YogEnv* env, int8_t* dest, YogVal val)
{
    SAVE_ARG(env, val);

    check_Fixnum_int8(env, val);
    *dest = VAL2INT(val);

    RETURN_VOID(env);
}

static void
write_argument_uint8(YogEnv* env, uint8_t* dest, YogVal val)
{
    SAVE_ARG(env, val);

    check_Fixnum_uint8(env, val);
    *dest = VAL2INT(val);

    RETURN_VOID(env);
}

static void
write_argument_uint16(YogEnv* env, uint16_t* dest, YogVal val)
{
    SAVE_ARG(env, val);

    check_Fixnum_uint16(env, val);
    *dest = VAL2INT(val);

    RETURN_VOID(env);
}

static void
write_argument_int16(YogEnv* env, int16_t* dest, YogVal val)
{
    SAVE_ARG(env, val);

    check_Fixnum_int16(env, val);
    *dest = VAL2INT(val);

    RETURN_VOID(env);
}

static void
write_argument_pointer(YogEnv* env, void** ptr, YogVal arg_type, YogVal val)
{
    SAVE_ARGS2(env, arg_type, val);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);
    if (!IS_PTR(arg_type) || (BASIC_OBJ_TYPE(arg_type) != TYPE_STRUCT_CLASS)) {
        YogError_raise_TypeError(env, "Argument type must be StructClass, not %C", arg_type);
    }
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "Argument must be Struct, not %C", val);
    }
    klass = YogVal_get_class(env, val);
    if (klass != arg_type) {
        ID name = PTR_AS(YogClass, arg_type)->name;
        YogError_raise_TypeError(env, "Argument must be %I, not %C", name, val);
    }
    *ptr = PTR_AS(Struct, val)->data;
    RETURN_VOID(env);
}

static void
write_argument(YogEnv* env, void* dest, YogVal arg_type, YogVal val)
{
    SAVE_ARGS2(env, arg_type, val);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    if (!IS_SYMBOL(arg_type)) {
        write_argument_pointer(env, (void**)dest, arg_type, val);
        RETURN_VOID(env);
    }

    s = YogVM_id2name(env, env->vm, VAL2ID(arg_type));
    if (strcmp(STRING_CSTR(s), "uint8") == 0) {
        write_argument_uint8(env, (uint8_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "int8") == 0) {
        write_argument_int8(env, (int8_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "uint16") == 0) {
        write_argument_uint16(env, (uint16_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "int16") == 0) {
        write_argument_int16(env, (int16_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "uint32") == 0) {
        write_argument_uint32(env, (uint32_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "int32") == 0) {
        write_argument_int32(env, (int32_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "uint64") == 0) {
        write_argument_uint64(env, (uint64_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "int64") == 0) {
        write_argument_int64(env, (int64_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "float") == 0) {
        write_argument_float(env, (float*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "double") == 0) {
        write_argument_double(env, (double*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "uchar") == 0) {
        write_argument_uint8(env, (uint8_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "char") == 0) {
        write_argument_int8(env, (int8_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "ushort") == 0) {
        write_argument_uint16(env, (uint16_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "short") == 0) {
        write_argument_int16(env, (int16_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "uint") == 0) {
        write_argument_uint32(env, (uint32_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "int") == 0) {
        write_argument_int32(env, (int32_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "ulong") == 0) {
        write_argument_uint32(env, (uint32_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "long") == 0) {
        write_argument_int32(env, (int32_t*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "longdouble") == 0) {
        write_argument_long_double(env, (long double*)dest, val);
    }
    else if (strcmp(STRING_CSTR(s), "pointer") == 0) {
        write_argument_uint32(env, (uint32_t*)dest, val);
    }
    else {
        YogError_raise_ValueError(env, "Unknown argument type");
        /* NOTREACHED */
    }

    RETURN_VOID(env);
}

static YogVal
LibFunc_do(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS4(env, callee, vararg, varkwarg, blockarg);
    YogVal arg_type = YUNDEF;
    PUSH_LOCAL(env, arg_type);

    uint_t nargs = PTR_AS(LibFunc, callee)->nargs;
    if (posargc != nargs) {
        YogError_raise_ValueError(env, "%u positional argument(s) required, not %u", nargs, posargc);
    }
    void** values = (void**)YogSysdeps_alloca(sizeof(void*) * nargs);
    ffi_type** arg_types = PTR_AS(LibFunc, callee)->cif.arg_types;
    uint_t i;
    for (i = 0; i < nargs; i++) {
        ffi_type* ffi_arg_type = arg_types[i];
        void* value = YogSysdeps_alloca(type2size(env, ffi_arg_type));
        arg_type = PTR_AS(LibFunc, callee)->arg_types[i];
        write_argument(env, value, arg_type, posargs[i]);
        values[i] = value;
    }

    void* rvalue = NULL;
    ffi_type* rtype = PTR_AS(LibFunc, callee)->cif.rtype;
    if (rtype != &ffi_type_void) {
        rvalue = YogSysdeps_alloca(type2size(env, rtype));
    }

    ffi_call(&PTR_AS(LibFunc, callee)->cif, PTR_AS(LibFunc, callee)->f, rvalue, values);

    if (rtype == &ffi_type_uint8) {
        RETURN(env, YogVal_from_unsigned_int(env, *((uint8_t*)rvalue)));
    }
    else if (rtype == &ffi_type_sint8) {
        RETURN(env, YogVal_from_int(env, *((int8_t*)rvalue)));
    }
    else if (rtype == &ffi_type_uint16) {
        RETURN(env, YogVal_from_unsigned_int(env, *((uint16_t*)rvalue)));
    }
    else if (rtype == &ffi_type_sint16) {
        RETURN(env, YogVal_from_int(env, *((int16_t*)rvalue)));
    }
    else if (rtype == &ffi_type_uint32) {
        RETURN(env, YogVal_from_unsigned_int(env, *((uint32_t*)rvalue)));
    }
    else if (rtype == &ffi_type_sint32) {
        RETURN(env, YogVal_from_int(env, *((int32_t*)rvalue)));
    }
    else if (rtype == &ffi_type_uint64) {
        RETURN(env, YogVal_from_unsigned_long_long(env, *((uint64_t*)rvalue)));
    }
    else if (rtype == &ffi_type_sint64) {
        RETURN(env, YogVal_from_long_long(env, *((int64_t*)rvalue)));
    }
    else if (rtype == &ffi_type_float) {
        RETURN(env, YogFloat_from_float(env, *((float*)rvalue)));
    }
    else if (rtype == &ffi_type_double) {
        RETURN(env, YogFloat_from_float(env, *((double*)rvalue)));
    }
    else if (rtype == &ffi_type_uchar) {
        RETURN(env, YogVal_from_unsigned_int(env, *((unsigned char*)rvalue)));
    }
    else if (rtype == &ffi_type_schar) {
        RETURN(env, YogVal_from_int(env, *((signed char*)rvalue)));
    }
    else if (rtype == &ffi_type_ushort) {
        RETURN(env, YogVal_from_unsigned_int(env, *((unsigned short*)rvalue)));
    }
    else if (rtype == &ffi_type_sshort) {
        RETURN(env, YogVal_from_int(env, *((short*)rvalue)));
    }
    else if (rtype == &ffi_type_uint) {
        RETURN(env, YogVal_from_unsigned_int(env, *((unsigned int*)rvalue)));
    }
    else if (rtype == &ffi_type_sint) {
        RETURN(env, YogVal_from_int(env, *((int*)rvalue)));
    }
    else if (rtype == &ffi_type_ulong) {
        RETURN(env, YogVal_from_unsigned_int(env, *((unsigned long*)rvalue)));
    }
    else if (rtype == &ffi_type_slong) {
        RETURN(env, YogVal_from_int(env, *((long*)rvalue)));
    }
    else if (rtype == &ffi_type_longdouble) {
        RETURN(env, YogFloat_from_float(env, *((long double*)rvalue)));
    }
    else if (rtype == &ffi_type_pointer) {
        RETURN(env, YogVal_from_unsigned_int(env, *((unsigned int*)rvalue)));
    }

    RETURN(env, YNIL);
}

static YogVal
LibFunc_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    return LibFunc_do(env, callee, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static void
LibFunc_exec(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS4(env, callee, vararg, varkwarg, blockarg);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = LibFunc_do(env, callee, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
    YogEval_push_returned_value(env, env->frame, retval);

    RETURN_VOID(env);
}

static YogVal
Struct_get(YogEnv* env, YogVal self, YogVal field)
{
    SAVE_ARGS2(env, self, field);
    YogVal s = YUNDEF;
    YogVal val = YUNDEF;
    YogVal type = YUNDEF;
    PUSH_LOCALS3(env, s, val, type);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "self must be Struct, not %C", self);
    }
    if (!IS_PTR(field) || (BASIC_OBJ_TYPE(field) != TYPE_FIELD)) {
        YogError_raise_TypeError(env, "Attribute must be Field, not %C", field);
    }

    void* ptr = PTR_AS(Struct, self)->data + PTR_AS(Field, field)->offset;
    type = PTR_AS(Field, field)->type;
    if (type == env->vm->cString) {
        char* p = *((char**)ptr);
        RETURN(env, p == NULL ? YNIL : YogString_from_str(env, p));
    }

    YOG_ASSERT(env, IS_SYMBOL(type), "Invalid FFI field type");
    s = YogVM_id2name(env, env->vm, VAL2ID(PTR_AS(Field, field)->type));
    const char* t = STRING_CSTR(s);
    if (strcmp(t, "uint8") == 0) {
        val = INT2VAL(*((uint8_t*)ptr));
    }
    else if (strcmp(t, "int8") == 0) {
        val = INT2VAL(*((int8_t*)ptr));
    }
    else if (strcmp(t, "uint16") == 0) {
        val = INT2VAL(*((uint16_t*)ptr));
    }
    else if (strcmp(t, "int16") == 0) {
        val = INT2VAL(*((int16_t*)ptr));
    }
    else if (strcmp(t, "uint32") == 0) {
        val = YogVal_from_unsigned_int(env, *((uint32_t*)ptr));
    }
    else if (strcmp(t, "int32") == 0) {
        val = YogVal_from_int(env, *((int32_t*)ptr));
    }
    else if (strcmp(t, "uint64") == 0) {
        val = YogVal_from_unsigned_long_long(env, *((uint64_t*)ptr));
    }
    else if (strcmp(t, "int64") == 0) {
        val = YogVal_from_long_long(env, *((int64_t*)ptr));
    }
    else if (strcmp(t, "float") == 0) {
        val = YogFloat_from_float(env, *((float*)ptr));
    }
    else if (strcmp(t, "double") == 0) {
        val = YogFloat_from_float(env, *((double*)ptr));
    }
    else if (strcmp(t, "uchar") == 0) {
        val = INT2VAL(*((uint8_t*)ptr));
    }
    else if (strcmp(t, "char") == 0) {
        val = INT2VAL(*((int8_t*)ptr));
    }
    else if (strcmp(t, "ushort") == 0) {
        val = INT2VAL(*((uint16_t*)ptr));
    }
    else if (strcmp(t, "short") == 0) {
        val = INT2VAL(*((int16_t*)ptr));
    }
    else if (strcmp(t, "uint") == 0) {
        val = YogVal_from_unsigned_int(env, *((uint32_t*)ptr));
    }
    else if (strcmp(t, "int") == 0) {
        val = YogVal_from_int(env, *((int32_t*)ptr));
    }
    else if (strcmp(t, "ulong") == 0) {
        val = YogVal_from_unsigned_int(env, *((uint32_t*)ptr));
    }
    else if (strcmp(t, "long") == 0) {
        val = YogVal_from_int(env, *((int32_t*)ptr));
    }
    else if (strcmp(t, "longdouble") == 0) {
        val = YogFloat_from_float(env, *((long double*)ptr));
    }
    else if (strcmp(t, "pointer") == 0) {
        val = YogVal_from_unsigned_int(env, *((uint32_t*)ptr));
    }
    else {
        YogError_raise_ValueError(env, "unknown type - %S", s);
        /* NOTREACHED */
    }

    RETURN(env, val);
}

static void
Field_exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    val = Struct_get(env, obj, attr);
    YogScriptFrame_push_stack(env, env->frame, val);

    RETURN_VOID(env);
}

static YogVal
Field_call_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    val = Struct_get(env, obj, attr);

    RETURN(env, val);
}

#define STRUCT_WRITE_DATA(type, obj, attr, val) do { \
    char* data = PTR_AS(Struct, (obj))->data; \
    uint_t offset = PTR_AS(Field, (attr))->offset; \
    *((type*)(data + offset)) = (val); \
} while (0)

static void
Struct_write_uint8(YogEnv* env, YogVal self, YogVal field, YogVal val)
{
    SAVE_ARGS3(env, self, field, val);

    check_Fixnum_uint8(env, val);
    STRUCT_WRITE_DATA(uint8_t, self, field, VAL2INT(val));

    RETURN_VOID(env);
}

static void
Struct_write_int8(YogEnv* env, YogVal self, YogVal field, YogVal val)
{
    SAVE_ARGS3(env, self, field, val);

    check_Fixnum_int8(env, val);
    STRUCT_WRITE_DATA(int8_t, self, field, VAL2INT(val));

    RETURN_VOID(env);
}

static void
Struct_write_uint16(YogEnv* env, YogVal self, YogVal field, YogVal val)
{
    SAVE_ARGS3(env, self, field, val);

    check_Fixnum_uint16(env, val);
    STRUCT_WRITE_DATA(uint16_t, self, field, VAL2INT(val));

    RETURN_VOID(env);
}

static void
Struct_write_int16(YogEnv* env, YogVal self, YogVal field, YogVal val)
{
    SAVE_ARGS3(env, self, field, val);

    check_Fixnum_int16(env, val);
    STRUCT_WRITE_DATA(int16_t, self, field, VAL2INT(val));

    RETURN_VOID(env);
}

#define WRITE_POSITIVE_NUM(env, type, obj, field, val) do { \
    if (VAL2INT((val)) < 0) { \
        YogError_raise_ValueError((env), "Value must be greater or equal 0, not %d", VAL2INT((val))); \
    } \
    STRUCT_WRITE_DATA(type, (obj), (field), VAL2INT((val))); \
} while (0)

static void
Struct_write_uint32(YogEnv* env, YogVal self, YogVal field, YogVal val)
{
    SAVE_ARGS3(env, self, field, val);

    if (IS_FIXNUM(val)) {
        WRITE_POSITIVE_NUM(env, uint32_t, self, field, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_uint32(env, val);
        uint_t n = YogBignum_to_unsigned_type(env, val, "Value");
        STRUCT_WRITE_DATA(uint32_t, self, field, n);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
Struct_write_int32(YogEnv* env, YogVal self, YogVal field, YogVal val)
{
    SAVE_ARGS3(env, self, field, val);

    if (IS_FIXNUM(val)) {
        STRUCT_WRITE_DATA(int32_t, self, field, VAL2INT(val));
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_int32(env, val);
        int_t n = YogBignum_to_signed_type(env, val, "Value");
        STRUCT_WRITE_DATA(int32_t, self, field, n);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
Struct_write_uint64(YogEnv* env, YogVal self, YogVal field, YogVal val)
{
    SAVE_ARGS3(env, self, field, val);

    if (IS_FIXNUM(val)) {
        WRITE_POSITIVE_NUM(env, uint64_t, self, field, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_uint64(env, val);
        unsigned long long n = YogBignum_to_unsigned_long_long(env, val, "Value");
        STRUCT_WRITE_DATA(uint64_t, self, field, n);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
Struct_write_int64(YogEnv* env, YogVal self, YogVal field, YogVal val)
{
    SAVE_ARGS3(env, self, field, val);

    if (IS_FIXNUM(val)) {
        STRUCT_WRITE_DATA(int64_t, self, field, VAL2INT(val));
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_int64(env, val);
        long long n = YogBignum_to_long_long(env, val, "Value");
        STRUCT_WRITE_DATA(int64_t, self, field, n);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
Field_exec_descr_set(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    SAVE_ARGS3(env, attr, obj, val);
    YogVal s = YUNDEF;
    YogVal type = YUNDEF;
    PUSH_LOCALS2(env, s, type);
    if (!IS_PTR(attr) || (BASIC_OBJ_TYPE(attr) != TYPE_FIELD)) {
        YogError_raise_TypeError(env, "Attribute must be Field, not %C", attr);
    }
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "Object must be Struct, not %C", obj);
    }
    type = PTR_AS(Field, attr)->type;
    if (type == env->vm->cString) {
        YogError_raise_FFIError(env, "Writing String fields is not supported.");
    }

#define WRITE_FLOAT(type) do { \
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_FLOAT)) { \
        YogError_raise_TypeError(env, "Value must be Float, not %C", val); \
    } \
    STRUCT_WRITE_DATA(type, obj, attr, FLOAT_NUM(val)); \
} while (0)
    YOG_ASSERT(env, IS_SYMBOL(type), "Invalid field type");
    s = YogVM_id2name(env, env->vm, VAL2ID(type));
    const char* t = STRING_CSTR(s);
    if (strcmp(t, "uint8") == 0) {
        Struct_write_uint8(env, obj, attr, val);
    }
    else if (strcmp(t, "int8") == 0) {
        Struct_write_int8(env, obj, attr, val);
    }
    else if (strcmp(t, "uint16") == 0) {
        Struct_write_uint16(env, obj, attr, val);
    }
    else if (strcmp(t, "int16") == 0) {
        Struct_write_int16(env, obj, attr, val);
    }
    else if (strcmp(t, "uint32") == 0) {
        Struct_write_uint32(env, obj, attr, val);
    }
    else if (strcmp(t, "int32") == 0) {
        Struct_write_int32(env, obj, attr, val);
    }
    else if (strcmp(t, "uint64") == 0) {
        Struct_write_uint64(env, obj, attr, val);
    }
    else if (strcmp(t, "int64") == 0) {
        Struct_write_int64(env, obj, attr, val);
    }
    else if (strcmp(t, "float") == 0) {
        WRITE_FLOAT(float);
    }
    else if (strcmp(t, "double") == 0) {
        WRITE_FLOAT(double);
    }
    else if (strcmp(t, "uchar") == 0) {
        Struct_write_uint8(env, obj, attr, val);
    }
    else if (strcmp(t, "char") == 0) {
        Struct_write_int8(env, obj, attr, val);
    }
    else if (strcmp(t, "ushort") == 0) {
        Struct_write_uint16(env, obj, attr, val);
    }
    else if (strcmp(t, "short") == 0) {
        Struct_write_int16(env, obj, attr, val);
    }
    else if (strcmp(t, "uint") == 0) {
        Struct_write_uint32(env, obj, attr, val);
    }
    else if (strcmp(t, "int") == 0) {
        Struct_write_int32(env, obj, attr, val);
    }
    else if (strcmp(t, "ulong") == 0) {
        Struct_write_uint32(env, obj, attr, val);
    }
    else if (strcmp(t, "long") == 0) {
        Struct_write_int32(env, obj, attr, val);
    }
    else if (strcmp(t, "longdouble") == 0) {
        WRITE_FLOAT(long double);
    }
    else if (strcmp(t, "pointer") == 0) {
        Struct_write_uint32(env, obj, attr, val);
    }
    else {
        YogError_raise_ValueError(env, "unknown type - %S", s);
        /* NOTREACHED */
    }
#undef WRITE_FLOAT

    RETURN_VOID(env);
}

void
YogFFI_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cLib = YUNDEF;
    YogVal cLibFunc = YUNDEF;
    YogVal cStructClassClass = YUNDEF;
    YogVal cStructClass = YUNDEF;
    YogVal cField = YUNDEF;
    PUSH_LOCALS5(env, cLib, cLibFunc, cStructClassClass, cStructClass, cField);
    YogVM* vm = env->vm;

    cLib = YogClass_new(env, "Lib", vm->cObject);
    YogClass_define_allocator(env, cLib, Lib_alloc);
    YogClass_define_method(env, cLib, pkg, "load_func", load_func);
    vm->cLib = cLib;
    cLibFunc = YogClass_new(env, "LibFunc", vm->cObject);
    YogClass_define_caller(env, cLibFunc, LibFunc_call);
    YogClass_define_executor(env, cLibFunc, LibFunc_exec);
    vm->cLibFunc = cLibFunc;

    cStructClassClass = YogClass_new(env, "StructClassClass", vm->cClass);
    YogClass_define_method(env, cStructClassClass, pkg, "new", StructClassClass_new);
    vm->cStructClassClass = cStructClassClass;
    cStructClass = YogClass_new(env, "StructClass", vm->cClass);
    YogGC_UPDATE_PTR(env, PTR_AS(YogBasicObj, cStructClass), klass, cStructClassClass);
    vm->cStructClass = cStructClass;
    cField = YogClass_new(env, "Field", vm->cObject);
    YogClass_define_allocator(env, cField, Field_alloc);
    YogClass_define_descr_get_executor(env, cField, Field_exec_descr_get);
    YogClass_define_descr_get_caller(env, cField, Field_call_descr_get);
    YogClass_define_descr_set_executor(env, cField, Field_exec_descr_set);
    vm->cField = cField;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
