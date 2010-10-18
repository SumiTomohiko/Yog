#include "yog/config.h"
#include <ctype.h>
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
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/float.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/sprintf.h"
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

struct FieldBase {
    struct YogBasicObj base;
    uint_t offset;
};

typedef struct FieldBase FieldBase;

struct Field {
    struct FieldBase base;
    ID type;
};

typedef struct Field Field;

#define TYPE_FIELD TO_TYPE(Field_alloc)

struct StructField {
    struct FieldBase base;
    YogVal klass;
};

typedef struct StructField StructField;

#define TYPE_STRUCT_FIELD TO_TYPE(StructField_alloc)

struct ArrayField {
    struct FieldBase base;
    ID type;
    int_t size;
};

typedef struct ArrayField ArrayField;

#define TYPE_ARRAY_FIELD TO_TYPE(ArrayField_alloc)

struct StringField {
    struct FieldBase base;
    YogVal encoding;
};

typedef struct StringField StringField;

#define TYPE_STRING_FIELD TO_TYPE(StringField_alloc)

struct BufferField {
    struct FieldBase base;
    uint_t buffer_index;
};

typedef struct BufferField BufferField;

#define TYPE_BUFFER_FIELD TO_TYPE(BufferField_alloc)

struct FieldArray {
    struct YogBasicObj base;
    YogVal st;
    YogVal field;
};

typedef struct FieldArray FieldArray;

#define TYPE_FIELD_ARRAY TO_TYPE(FieldArray_alloc)

struct StructClass {
    struct YogClass base;
    uint_t size;
    uint_t buffers_num;
    uint_t first_field_size;
};

typedef struct StructClass StructClass;

#define TYPE_STRUCT_CLASS TO_TYPE(StructClassClass_new)
static YogVal StructClassClass_new(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block);

struct Struct {
    struct YogBasicObj base;
    void* data;
    BOOL own;
    YogVal top;
    /**
     * A structure refered by Struct::data may have some buffers. These buffers
     * are members of Buffer objects. So Struct objects must keep these Buffer
     * objects to avoid GC.
     */
    YogVal buffers[0];
};

typedef struct Struct Struct;

#define TYPE_STRUCT TO_TYPE(StructBase_alloc)

static YogVal StructBase_alloc(YogEnv* env, YogVal klass);

struct Int {
    struct YogBasicObj base;
    int value;
};

typedef struct Int Int;

#define TYPE_INT TO_TYPE(Int_alloc)

struct Buffer {
    struct YogBasicObj base;
    uint_t size;
    void* ptr;
};

typedef struct Buffer Buffer;

#define TYPE_BUFFER TO_TYPE(Buffer_alloc)
#define CHECK_SELF_BUFFER do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_BUFFER)) { \
        YogError_raise_TypeError(env, "self must be Buffer, not %C", self); \
    } \
} while (0)
#define CHECK_SELF_POINTER do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_POINTER)) { \
        YogError_raise_TypeError(env, "self must be Pointer, not %C", self); \
    } \
} while (0)

struct Pointer {
    struct YogBasicObj base;
    void* ptr;
};

typedef struct Pointer Pointer;

#define TYPE_POINTER TO_TYPE(Pointer_alloc)

static void
FieldBase_init(YogEnv* env, YogVal self, type_t type, YogVal klass)
{
    YogBasicObj_init(env, self, type, 0, klass);
    PTR_AS(FieldBase, self)->offset = 0;
}

static void
StructField_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    StructField* field = (StructField*)ptr;
    YogGC_KEEP(env, field, klass, keeper, heap);
}

static YogVal
StructField_alloc(YogEnv* env, YogVal klass)
{
    YogVal field = ALLOC_OBJ(env, StructField_keep_children, NULL, StructField);
    FieldBase_init(env, field, TYPE_STRUCT_FIELD, klass);
    PTR_AS(StructField, field)->klass = YUNDEF;
    return field;
}

static YogVal
StructField_new(YogEnv* env, uint_t offset, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal field = StructField_alloc(env, env->vm->cStructField);
    PTR_AS(FieldBase, field)->offset = offset;
    YogGC_UPDATE_PTR(env, PTR_AS(StructField, field), klass, klass);

    RETURN(env, field);
}

static YogVal
ArrayField_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    field = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, ArrayField);
    FieldBase_init(env, field, TYPE_ARRAY_FIELD, klass);
    PTR_AS(ArrayField, field)->type = INVALID_ID;
    PTR_AS(ArrayField, field)->size = 0;

    RETURN(env, field);
}

static void
StringField_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    StringField* field = (StringField*)ptr;
    YogGC_KEEP(env, field, encoding, keeper, heap);
}

static YogVal
StringField_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    field = ALLOC_OBJ(env, StringField_keep_children, NULL, StringField);
    FieldBase_init(env, field, TYPE_STRING_FIELD, klass);
    PTR_AS(StringField, field)->encoding = YNIL;

    RETURN(env, field);
}

static YogVal
BufferField_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    field = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, BufferField);
    FieldBase_init(env, field, TYPE_BUFFER_FIELD, klass);
    PTR_AS(BufferField, field)->buffer_index = 0;

    RETURN(env, field);
}

static void
FieldArray_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    FieldArray* array = (FieldArray*)ptr;
#define KEEP(member) YogGC_KEEP(env, array, member, keeper, heap)
    KEEP(st);
    KEEP(field);
#undef KEEP
}

static YogVal
FieldArray_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    array = ALLOC_OBJ(env, FieldArray_keep_children, NULL, FieldArray);
    YogBasicObj_init(env, array, TYPE_FIELD_ARRAY, 0, klass);
    PTR_AS(FieldArray, array)->st = YUNDEF;
    PTR_AS(FieldArray, array)->field = YUNDEF;

    RETURN(env, array);
}

static YogVal
Pointer_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal ptr = YUNDEF;
    PUSH_LOCAL(env, ptr);

    ptr = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Pointer);
    YogBasicObj_init(env, ptr, TYPE_POINTER, 0, klass);
    PTR_AS(Pointer, ptr)->ptr = NULL;

    RETURN(env, ptr);
}

static void
Buffer_finalize(YogEnv* env, void* ptr)
{
    Buffer* buf = (Buffer*)ptr;
    YogGC_free(env, buf->ptr, buf->size);
}

static YogVal
Buffer_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal buf = YUNDEF;
    PUSH_LOCAL(env, buf);

    buf = ALLOC_OBJ(env, YogBasicObj_keep_children, Buffer_finalize, Buffer);
    YogBasicObj_init(env, buf, TYPE_BUFFER, 0, klass);
    PTR_AS(Buffer, buf)->size = 0;
    PTR_AS(Buffer, buf)->ptr = NULL;

    RETURN(env, buf);
}

static YogVal
Int_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal int_ = YUNDEF;
    PUSH_LOCAL(env, int_);

    int_ = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Int);
    YogBasicObj_init(env, int_, TYPE_INT, 0, klass);
    PTR_AS(Int, int_)->value = 0;

    RETURN(env, int_);
}

static YogVal
Field_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Field);
    FieldBase_init(env, obj, TYPE_FIELD, klass);
    PTR_AS(Field, obj)->type = INVALID_ID;

    RETURN(env, obj);
}

static YogVal
Field_new(YogEnv* env, uint_t offset, ID type)
{
    SAVE_LOCALS(env);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    field = Field_alloc(env, env->vm->cField);
    PTR_AS(FieldBase, field)->offset = offset;
    PTR_AS(Field, field)->type = type;

    RETURN(env, field);
}

static YogVal
StringField_new(YogEnv* env, uint_t offset, YogVal encoding)
{
    YogHandle* h = VAL2HDL(env, encoding);
    YogVal field = StringField_alloc(env, env->vm->cStringField);
    PTR_AS(FieldBase, field)->offset = offset;
    YogGC_UPDATE_PTR(env, PTR_AS(StringField, field), encoding, HDL2VAL(h));
    return field;
}

static YogVal
BufferField_new(YogEnv* env, uint_t offset, uint_t buffer_index)
{
    SAVE_LOCALS(env);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    field = BufferField_alloc(env, env->vm->cBufferField);
    PTR_AS(FieldBase, field)->offset = offset;
    PTR_AS(BufferField, field)->buffer_index = buffer_index;

    RETURN(env, field);
}

static YogVal
create_parameterized_field(YogEnv* env, YogVal field, uint_t offset)
{
    YOG_ASSERT(env, IS_PTR(field), "Invalid field (0x%08x)", field);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(field) == TYPE_ARRAY, "Invalid field (0x%08x)", BASIC_OBJ_TYPE(field));
    YogHandle* h = VAL2HDL(env, field);
    uint_t size = YogArray_size(env, field);
    if (size != 2) {
        const char* fmt = "Parameterized field size must be 2, not %u";
        YogError_raise_ValueError(env, fmt, size);
        /* NOTREACHED */
    }
    YogVal type = YogArray_at(env, field, 0);
    if (!IS_SYMBOL(type)) {
        const char* fmt = "Parameterized field type must be Symbol, not %C";
        YogError_raise_ValueError(env, fmt, type);
        /* NOTREACHED */
    }
    ID id = VAL2ID(type);
    if (id != YogVM_intern(env, env->vm, "string")) {
        const char* fmt = "Parameterized field type must be \'string, not %I";
        YogError_raise_ValueError(env, fmt, id);
        /* NOTREACHED */
    }
    YogVal enc = YogArray_at(env, HDL2VAL(h), 1);
    if (!IS_PTR(enc) || (BASIC_OBJ_TYPE(enc) != TYPE_ENCODING)) {
        const char* fmt = "Parameter of \'string must be Encoding, not %C";
        YogError_raise_ValueError(env, fmt, enc);
        /* NOTREACHED */
    }
    return StringField_new(env, offset, enc);
}

static YogVal
create_scalar_field(YogEnv* env, YogVal type, uint_t offset, uint_t buffer_index)
{
    SAVE_ARG(env, type);
    YogVM* vm = env->vm;

    if (IS_SYMBOL(type)) {
        RETURN(env, Field_new(env, offset, VAL2ID(type)));
    }
#define RAISE_TYPE_ERROR do { \
    const char* fmt = "Type must be Symbol, Array or Buffer, not %C"; \
    YogError_raise_TypeError(env, fmt, type); \
} while (0)
    if (!IS_PTR(type)) {
        RAISE_TYPE_ERROR;
    }
    if (BASIC_OBJ_TYPE(type) == TYPE_STRUCT_CLASS) {
        RETURN(env, StructField_new(env, offset, type));
    }
    if (BASIC_OBJ_TYPE(type) == TYPE_ARRAY) {
        RETURN(env, create_parameterized_field(env, type, offset));
    }
    if (type == vm->cBuffer) {
        RETURN(env, BufferField_new(env, offset, buffer_index));
    }
    RAISE_TYPE_ERROR;
    /* NOTREACHED */
#undef RAISE_TYPE_ERROR
    RETURN(env, YUNDEF);
}

static YogVal
ArrayField_new(YogEnv* env, YogVal type, int_t size, uint_t offset)
{
    SAVE_ARG(env, type);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);
    if (!IS_SYMBOL(type)) {
        YogError_raise_TypeError(env, "Type of array must be Symbol, not %C", type);
    }

    field = ArrayField_alloc(env, env->vm->cArrayField);
    PTR_AS(FieldBase, field)->offset = offset;
    PTR_AS(ArrayField, field)->type = VAL2ID(type);
    PTR_AS(ArrayField, field)->size = size;

    RETURN(env, field);
}

static YogVal
create_field(YogEnv* env, YogVal type, int_t size, uint_t offset, uint_t buffer_index)
{
    if (size == -1) {
        return create_scalar_field(env, type, offset, buffer_index);
    }
    return ArrayField_new(env, type, size, offset);
}

static void
StructClass_set_field(YogEnv* env, YogVal self, uint_t index, YogVal name, YogVal type, int_t size, uint_t offset, uint_t buffer_index)
{
    SAVE_ARGS3(env, self, name, type);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    field = create_field(env, type, size, offset, buffer_index);
    if (!IS_SYMBOL(name)) {
        YogError_raise_TypeError(env, "Name must be Symbol, not %C", name);
    }
    YogObj_set_attr_id(env, self, VAL2ID(name), field);

    RETURN_VOID(env);
}

static void
StructClass_init(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);

    YogClass_init(env, self, TYPE_STRUCT_CLASS, env->vm->cStructClass);
    PTR_AS(StructClass, self)->size = 0;
    YogGC_UPDATE_PTR(env, PTR_AS(YogClass, self), super, env->vm->cStructBase);

    RETURN_VOID(env);
}

static uint_t
id2size(YogEnv* env, ID type)
{
    const char* s = BINARY_CSTR(YogVM_id2bin(env, env->vm, type));
    uint_t size;
    if (strcmp(s, "uint8") == 0) {
        size = sizeof(uint8_t);
    }
    else if (strcmp(s, "int8") == 0) {
        size = sizeof(int8_t);
    }
    else if (strcmp(s, "uint16") == 0) {
        size = sizeof(uint16_t);
    }
    else if (strcmp(s, "int16") == 0) {
        size = sizeof(int16_t);
    }
    else if (strcmp(s, "uint32") == 0) {
        size = sizeof(uint32_t);
    }
    else if (strcmp(s, "int32") == 0) {
        size = sizeof(int32_t);
    }
    else if (strcmp(s, "uint64") == 0) {
        size = sizeof(uint64_t);
    }
    else if (strcmp(s, "int64") == 0) {
        size = sizeof(int64_t);
    }
    else if (strcmp(s, "float") == 0) {
        size = sizeof(float);
    }
    else if (strcmp(s, "double") == 0) {
        size = sizeof(double);
    }
    else if (strcmp(s, "uchar") == 0) {
        size = sizeof(unsigned char);
    }
    else if (strcmp(s, "char") == 0) {
        size = sizeof(char);
    }
    else if (strcmp(s, "ushort") == 0) {
        size = sizeof(unsigned short);
    }
    else if (strcmp(s, "short") == 0) {
        size = sizeof(short);
    }
    else if (strcmp(s, "uint") == 0) {
        size = sizeof(unsigned int);
    }
    else if (strcmp(s, "int") == 0) {
        size = sizeof(int);
    }
    else if (strcmp(s, "ulong") == 0) {
        size = sizeof(unsigned long);
    }
    else if (strcmp(s, "long") == 0) {
        size = sizeof(long);
    }
    else if (strcmp(s, "longdouble") == 0) {
        size = sizeof(long double);
    }
    else if (strcmp(s, "pointer") == 0) {
        size = sizeof(void*);
    }
    else if (strcmp(s, "string") == 0) {
        size = sizeof(char*);
    }
    else {
        YogError_raise_ValueError(env, "Unknown type for size - %I", type);
        /* NOTREACHED */
        /**
         * gcc complains "‘size’ may be used uninitialized in this function"
         * without the following assignment.
         */
        size = 0;
    }

    return size;
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
        /**
         * gcc complains "‘size’ may be used uninitialized in this function"
         * without the following assignment.
         */
        size = 0;
    }

    RETURN(env, size);
}

static uint_t
get_size(YogEnv* env, YogVal type)
{
    if (IS_PTR(type) && (BASIC_OBJ_TYPE(type) == TYPE_STRUCT_CLASS)) {
        return PTR_AS(StructClass, type)->size;
    }
    return IS_SYMBOL(type) ? id2size(env, VAL2ID(type)) : sizeof(void*);
}

static YogVal
get_fields_of_anonymous_struct(YogEnv* env, YogVal type)
{
    if (YogArray_size(env, type) < 1) {
        YogError_raise_ValueError(env, "struct field have no fields");
    }
    YogVal fields = YogArray_at(env, type, 1);
    if (!IS_PTR(fields) || (BASIC_OBJ_TYPE(fields) != TYPE_ARRAY)) {
        YogError_raise_TypeError(env, "Fields must be Array, not %C", fields);
    }
    return fields;
}

static uint_t
get_alignment_unit(YogEnv* env, YogVal type)
{
    if (IS_PTR(type) && (BASIC_OBJ_TYPE(type) == TYPE_STRUCT_CLASS)) {
        return PTR_AS(StructClass, type)->first_field_size;
    }
    return get_size(env, type);
}

static uint_t
align_offset(YogEnv* env, YogVal type, uint_t offset)
{
    SAVE_ARG(env, type);
    uint_t size = get_alignment_unit(env, type);
    uint_t alignment = sizeof(void*) < size ? sizeof(void*) : size;
#define ALIGN(offset, alignment) \
                            (((offset) + (alignment) - 1) & ~((alignment) - 1))
    RETURN(env, ALIGN(offset, alignment));
#undef ALIGN
}

static YogVal
StructBase_get_size(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal size = YUNDEF;
    YogVal klass = YUNDEF;
    PUSH_LOCALS2(env, size, klass);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_size", params, args, kw);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "self must be Struct, not %C", self);
    }
    klass = YogVal_get_class(env, self);
    size = YogVal_from_unsigned_int(env, PTR_AS(StructClass, klass)->size);
    RETURN(env, size);
}

static YogVal
StructBase_init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal klass = YUNDEF;
    YogVal ptr = YNIL;
    PUSH_LOCALS2(env, klass, ptr);
    YogCArg params[] = { { "|", NULL }, { "ptr", &ptr }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "self must be Struct, not %C", self);
    }
    if (!IS_NIL(ptr) && (!IS_PTR(ptr) || (BASIC_OBJ_TYPE(ptr) != TYPE_POINTER))) {
        YogError_raise_TypeError(env, "ptr must be Nil or Pointer, not %C", ptr);
    }

    if (!IS_NIL(ptr)) {
        PTR_AS(Struct, self)->data = PTR_AS(Pointer, ptr)->ptr;
        PTR_AS(Struct, self)->own = FALSE;
        RETURN(env, self);
    }
    klass = YogVal_get_class(env, self);
    uint_t size = PTR_AS(StructClass, klass)->size;
    PTR_AS(Struct, self)->data = YogGC_malloc(env, size);
    bzero(PTR_AS(Struct, self)->data, size);

    RETURN(env, self);
}

static YogVal build_StructClass(YogEnv*, ID, YogVal);

static YogVal
create_anonymous_StructClass(YogEnv* env, YogVal type)
{
    YogVal fields = get_fields_of_anonymous_struct(env, type);
    return build_StructClass(env, INVALID_ID, fields);
}

static void
get_field_type_and_size(YogEnv* env, YogVal type, YogVal* field_type, int_t* psize)
{
    SAVE_ARG(env, type);
    YogVal array_type = YUNDEF;
    YogVal array_size = YUNDEF;
    PUSH_LOCALS2(env, array_type, array_size);
    if (IS_SYMBOL(type)) {
        *field_type = type;
        *psize = -1; /* -1 indicates that this field is scalar */
        RETURN_VOID(env);
    }
#define RAISE_TYPE_ERROR do { \
    const char* fmt = "Field must be Symbol or Array, not %C"; \
    YogError_raise_TypeError(env, fmt, type); \
} while (0)
    if (!IS_PTR(type)) {
        RAISE_TYPE_ERROR;
        RETURN_VOID(env);
    }
    YogVM* vm = env->vm;
    if ((type == vm->cBuffer) || (BASIC_OBJ_TYPE(type) == TYPE_STRUCT_CLASS)) {
        *field_type = type;
        *psize = -1;
        RETURN_VOID(env);
    }
    if (BASIC_OBJ_TYPE(type) != TYPE_ARRAY) {
        RAISE_TYPE_ERROR;
    }
#undef RAISE_TYPE_ERROR
    uint_t size = YogArray_size(env, type);
    if (size != 2) {
        const char* fmt = "Array field must be length of two, not %u";
        YogError_raise_ValueError(env, fmt, size);
        /* NOTREACHED */
    }
    array_type = YogArray_at(env, type, 0);
    if (!IS_SYMBOL(array_type)) {
        const char* fmt = "Array field type must be Symbol, not %C";
        YogError_raise_TypeError(env, fmt, array_type);
        /* NOTREACHED */
    }
    if (VAL2ID(array_type) == YogVM_intern(env, env->vm, "string")) {
        *field_type = type;
        *psize = -1;
        RETURN_VOID(env);
    }
    if (VAL2ID(array_type) == YogVM_intern(env, env->vm, "struct")) {
        *field_type = create_anonymous_StructClass(env, type);
        *psize = -1;
        RETURN_VOID(env);
    }
    *field_type = array_type;
    array_size = YogArray_at(env, type, 1);
    if (!IS_FIXNUM(array_size)) {
        const char* fmt = "Array field size must be Fixnum, not %C";
        YogError_raise_TypeError(env, fmt, array_size);
        /* NOTREACHED */
    }
    if (VAL2INT(array_size) < 0) {
        const char* fmt = "Array field size must be positive, not %d";
        YogError_raise_ValueError(env, fmt, VAL2INT(array_size));
        /* NOTREACHED */
    }
    *psize = VAL2INT(array_size);
    RETURN_VOID(env);
}

static uint_t
get_first_field_size(YogEnv* env, YogVal fields)
{
    if (YogArray_size(env, fields) == 0) {
        return 0;
    }
    YogVal field = YogArray_at(env, fields, 0);
    if (YogArray_size(env, field) == 0) {
        YogError_raise_ValueError(env, "Field have no elements");
    }
    YogVal type = YogArray_at(env, field, 0);
    if (IS_SYMBOL(type) && (VAL2ID(type) == YogVM_intern(env, env->vm, "struct"))) {
        YogVal fields = get_fields_of_anonymous_struct(env, field);
        return get_first_field_size(env, fields);
    }
    return get_alignment_unit(env, type);
}

static YogVal
build_StructClass(YogEnv* env, ID name, YogVal fields)
{
    SAVE_ARG(env, fields);
    YogVal obj = YUNDEF;
    YogVal cBuffer = YUNDEF;
    YogVal field = YUNDEF;
    YogVal field_name = YUNDEF;
    YogVal field_type = YUNDEF;
    PUSH_LOCALS5(env, obj, cBuffer, field, field_name, field_type);

    obj = ALLOC_OBJ(env, YogClass_keep_children, NULL, StructClass);
    StructClass_init(env, obj);

    PTR_AS(YogClass, obj)->name = name;
    uint_t first_field_size = get_first_field_size(env, fields);
    PTR_AS(StructClass, obj)->first_field_size = first_field_size;

    uint_t offset = 0;
    uint_t buffers_num = 0;
    cBuffer = env->vm->cBuffer;
    uint_t fields_num = YogArray_size(env, fields);
    uint_t i;
    for (i = 0; i < fields_num; i++) {
        field = YogArray_at(env, fields, i);
        /**
         * gcc complains "‘field_size’ may be used uninitialized in this
         * function" without assignement initial value.
         */
        int_t field_size = 0;
        YogVal type = YogArray_at(env, field, 0);
        get_field_type_and_size(env, type, &field_type, &field_size);
        offset = align_offset(env, field_type, offset);
        field_name = YogArray_at(env, field, 1);
        StructClass_set_field(env, obj, i, field_name, field_type, field_size, offset, buffers_num);
        offset += get_size(env, field_type) * (field_size == -1 ? 1 : field_size);
        buffers_num += field_type == cBuffer ? 1 : 0;
    }
    PTR_AS(StructClass, obj)->size = offset;
    PTR_AS(StructClass, obj)->buffers_num = buffers_num;
    RETURN(env, obj);
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
    YogVal cBuffer = YUNDEF;
    PUSH_LOCALS7(env, name, obj, fields, field, field_type, field_name, cBuffer);
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

    ID id = YogVM_intern2(env, env->vm, name);
    RETURN(env, build_StructClass(env, id, fields));
}

static void
Struct_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    Struct* st = (Struct*)ptr;
    YogGC_KEEP(env, st, top, keeper, heap);

    YogVal klass = ((YogBasicObj*)st)->klass;
    YOG_ASSERT(env, BASIC_OBJ_TYPE(klass) == TYPE_STRUCT_CLASS, "Invalid class");
    uint_t buffers_num = PTR_AS(StructClass, klass)->buffers_num;
    uint_t i;
    for (i = 0; i < buffers_num; i++) {
        YogGC_KEEP(env, st, buffers[i], keeper, heap);
    }
}

static void
Struct_finalize(YogEnv* env, void* ptr)
{
    Struct* st = (Struct*)ptr;
    if (!st->own) {
        return;
    }
    YogVal klass = YogVal_get_class(env, PTR2VAL(st));
    YogGC_free(env, st->data, PTR_AS(StructClass, klass)->size);
}

static YogVal
StructBase_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(klass) == TYPE_STRUCT_CLASS, "invalid class");

    uint_t buffers_num = PTR_AS(StructClass, klass)->buffers_num;
    YogGC_check_multiply_overflow(env, buffers_num, sizeof(YogVal));
    obj = ALLOC_OBJ_ITEM(env, Struct_keep_children, Struct_finalize, Struct, buffers_num, YogVal);
    YogBasicObj_init(env, obj, TYPE_STRUCT, 0, klass);
    PTR_AS(Struct, obj)->data = NULL;
    PTR_AS(Struct, obj)->own = TRUE;
    PTR_AS(Struct, obj)->top = obj;
    uint_t i;
    for (i = 0; i < buffers_num; i++) {
        PTR_AS(Struct, obj)->buffers[i] = YNIL;
    }

    RETURN(env, obj);
}

static void
LibFunc_finalize(YogEnv* env, void* ptr)
{
    LibFunc* f = (LibFunc*)ptr;
    YogGC_free(env, f->cif.arg_types, sizeof(*f->cif.arg_types) * f->nargs);
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

    YogGC_check_multiply_overflow(env, nargs, sizeof(YogVal));
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
YogFFI_load_lib(YogEnv* env, YogHandle* path)
{
    LIB_HANDLE handle = YogMisc_load_lib(env, path);
    if (handle == NULL) {
        const char* fmt = "No library named \"%S\"";
        YogError_raise_ImportError(env, fmt, HDL2VAL(path));
        /* NOTREACHED */
    }
    YogVal lib = Lib_alloc(env, env->vm->cLib);
    PTR_AS(Lib, lib)->handle = handle;

    return lib;
}

static ffi_type*
map_id_type(YogEnv* env, ID type)
{
    const char* s = BINARY_CSTR(YogVM_id2bin(env, env->vm, type));
    ffi_type* cif_type;
    if (strcmp(s, "void") == 0) {
        cif_type = &ffi_type_void;
    }
    else if (strcmp(s, "uint8") == 0) {
        cif_type = &ffi_type_uint8;
    }
    else if (strcmp(s, "int8") == 0) {
        cif_type = &ffi_type_sint8;
    }
    else if (strcmp(s, "uint16") == 0) {
        cif_type = &ffi_type_uint16;
    }
    else if (strcmp(s, "int16") == 0) {
        cif_type = &ffi_type_sint16;
    }
    else if (strcmp(s, "uint32") == 0) {
        cif_type = &ffi_type_uint32;
    }
    else if (strcmp(s, "int32") == 0) {
        cif_type = &ffi_type_sint32;
    }
    else if (strcmp(s, "uint64") == 0) {
        cif_type = &ffi_type_uint64;
    }
    else if (strcmp(s, "int64") == 0) {
        cif_type = &ffi_type_sint64;
    }
    else if (strcmp(s, "float") == 0) {
        cif_type = &ffi_type_float;
    }
    else if (strcmp(s, "double") == 0) {
        cif_type = &ffi_type_double;
    }
    else if (strcmp(s, "uchar") == 0) {
        cif_type = &ffi_type_uchar;
    }
    else if (strcmp(s, "char") == 0) {
        cif_type = &ffi_type_schar;
    }
    else if (strcmp(s, "ushort") == 0) {
        cif_type = &ffi_type_ushort;
    }
    else if (strcmp(s, "short") == 0) {
        cif_type = &ffi_type_sshort;
    }
    else if (strcmp(s, "uint") == 0) {
        cif_type = &ffi_type_uint;
    }
    else if (strcmp(s, "int") == 0) {
        cif_type = &ffi_type_sint;
    }
    else if (strcmp(s, "ulong") == 0) {
        cif_type = &ffi_type_ulong;
    }
    else if (strcmp(s, "long") == 0) {
        cif_type = &ffi_type_slong;
    }
    else if (strcmp(s, "longdouble") == 0) {
        cif_type = &ffi_type_longdouble;
    }
    else if (strcmp(s, "pointer") == 0) {
        cif_type = &ffi_type_pointer;
    }
    else if (strcmp(s, "int_p") == 0) {
        cif_type = &ffi_type_pointer;
    }
    else if (strcmp(s, "pointer_p") == 0) {
        cif_type = &ffi_type_pointer;
    }
    else {
        YogError_raise_ValueError(env, "Unknown type - %I", type);
        /* NOTREACHED */
        /**
         * gcc complains "‘cif_type’ may be used uninitialized in this function"
         * without the following assignment.
         */
        cif_type = NULL;
    }

    return cif_type;
}

static ffi_type*
map_type(YogEnv* env, YogVal type)
{
    if (IS_SYMBOL(type)) {
        return map_id_type(env, VAL2ID(type));
    }
    YogVM* vm = env->vm;
    if ((IS_PTR(type) && ((BASIC_OBJ_TYPE(type) == TYPE_STRUCT_CLASS) || (BASIC_OBJ_TYPE(type) == TYPE_ARRAY))) || (type == vm->cBuffer)) {
        return &ffi_type_pointer;
    }
    const char* fmt = "Type must be Symbol, Array, Buffer or StructClass, not %C";
    YogError_raise_TypeError(env, fmt, type);
    /* NOTREACHED */
    return NULL;
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
        const char* fmt = "arg_types must be Array or nil, not %C";
        YogError_raise_TypeError(env, fmt, arg_types);
        /* NOTREACHED */
    }
    if (!IS_NIL(rtype) && !IS_SYMBOL(rtype)) {
        const char* fmt = "rtype must be Symbol or nil, not %C";
        YogError_raise_TypeError(env, fmt, rtype);
        /* NOTREACHED */
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
        YogError_raise_FFIError(env, "%s", map_ffi_error(env, status));
        /* NOTREACHED */
    }
    YogVal s = YogString_to_bin_in_default_encoding(env, VAL2HDL(env, name));
    void* p = YogSysdeps_get_proc(PTR_AS(Lib, self)->handle, BINARY_CSTR(s));
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
write_argument_Buffer(YogEnv* env, void** ptr, YogVal val)
{
    SAVE_ARG(env, val);
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_BUFFER)) {
        YogError_raise_TypeError(env, "Argument must be Buffer, not %C", val);
    }
    *ptr = PTR_AS(Buffer, val)->ptr;
    RETURN_VOID(env);
}

static void
write_argument_Struct(YogEnv* env, void** ptr, YogVal arg_type, YogVal val)
{
    SAVE_ARGS2(env, arg_type, val);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);
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
write_argument_pointer(YogEnv* env, void** ptr, YogVal val)
{
    SAVE_ARG(env, val);
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_POINTER)) {
        YogError_raise_TypeError(env, "Argument must be Pointer, not %C", val);
    }
    *ptr = PTR_AS(Pointer, val)->ptr;
    RETURN_VOID(env);
}

static void
write_argument_object(YogEnv* env, void** ptr, void* refered, YogVal arg_type, YogVal val)
{
    SAVE_ARGS2(env, arg_type, val);
    YogVM* vm = env->vm;
    if (arg_type == vm->cBuffer) {
        write_argument_Buffer(env, ptr, val);
        RETURN_VOID(env);
    }
#define RAISE_TYPE_ERROR do { \
    const char* fmt = "Argument type must be Array, Buffer or StructClass, not %C"; \
    YogError_raise_TypeError(env, fmt, arg_type); \
} while (0)
    if (!IS_PTR(arg_type)) {
        RAISE_TYPE_ERROR;
    }
    if (BASIC_OBJ_TYPE(arg_type) == TYPE_STRUCT_CLASS) {
        write_argument_Struct(env, ptr, arg_type, val);
        RETURN_VOID(env);
    }
    if (BASIC_OBJ_TYPE(arg_type) != TYPE_ARRAY) {
        RAISE_TYPE_ERROR;
    }
#undef RAISE_TYPE_ERROR
    if (YogArray_size(env, arg_type) != 2) {
        const char* fmt = "Parameterized type must have two elements";
        YogError_raise_ValueError(env, fmt);
        /* NOTREACHED */
    }
    YogHandle* enc = VAL2HDL(env, YogArray_at(env, arg_type, 1));
    YogVal bin = YogEncoding_conv_from_yog(env, enc, VAL2HDL(env, val));
    memcpy(refered, BINARY_CSTR(bin), BINARY_SIZE(bin));
    *ptr = refered;
    /* NOTREACHED */
    RETURN_VOID(env);
}

static void
Pointer_write(YogEnv* env, YogVal self, void** ptr)
{
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_POINTER)) {
        const char* fmt = "Argument must be Pointer, not %C";
        YogError_raise_TypeError(env, fmt, self);
        /* NOTREACHED */
    }
    *ptr = PTR_AS(Pointer, self)->ptr;
}

static void
Int_write(YogEnv* env, YogVal self, int* p)
{
    SAVE_ARG(env, self);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_INT)) {
        YogError_raise_TypeError(env, "Argument must be Int, not %C", self);
    }
    *p = PTR_AS(Int, self)->value;
    RETURN_VOID(env);
}

static void
write_argument(YogEnv* env, void* pvalue, void* refered, YogVal arg_type, YogVal val)
{
    SAVE_ARG(env, val);
    if (!IS_SYMBOL(arg_type)) {
        write_argument_object(env, (void**)pvalue, refered, arg_type, val);
        RETURN_VOID(env);
    }

    const char* s = BINARY_CSTR(YogVM_id2bin(env, env->vm, VAL2ID(arg_type)));
    if (strcmp(s, "uint8") == 0) {
        write_argument_uint8(env, (uint8_t*)pvalue, val);
    }
    else if (strcmp(s, "int8") == 0) {
        write_argument_int8(env, (int8_t*)pvalue, val);
    }
    else if (strcmp(s, "uint16") == 0) {
        write_argument_uint16(env, (uint16_t*)pvalue, val);
    }
    else if (strcmp(s, "int16") == 0) {
        write_argument_int16(env, (int16_t*)pvalue, val);
    }
    else if (strcmp(s, "uint32") == 0) {
        write_argument_uint32(env, (uint32_t*)pvalue, val);
    }
    else if (strcmp(s, "int32") == 0) {
        write_argument_int32(env, (int32_t*)pvalue, val);
    }
    else if (strcmp(s, "uint64") == 0) {
        write_argument_uint64(env, (uint64_t*)pvalue, val);
    }
    else if (strcmp(s, "int64") == 0) {
        write_argument_int64(env, (int64_t*)pvalue, val);
    }
    else if (strcmp(s, "float") == 0) {
        write_argument_float(env, (float*)pvalue, val);
    }
    else if (strcmp(s, "double") == 0) {
        write_argument_double(env, (double*)pvalue, val);
    }
    else if (strcmp(s, "uchar") == 0) {
        write_argument_uint8(env, (uint8_t*)pvalue, val);
    }
    else if (strcmp(s, "char") == 0) {
        write_argument_int8(env, (int8_t*)pvalue, val);
    }
    else if (strcmp(s, "ushort") == 0) {
        write_argument_uint16(env, (uint16_t*)pvalue, val);
    }
    else if (strcmp(s, "short") == 0) {
        write_argument_int16(env, (int16_t*)pvalue, val);
    }
    else if (strcmp(s, "uint") == 0) {
        write_argument_uint32(env, (uint32_t*)pvalue, val);
    }
    else if (strcmp(s, "int") == 0) {
        write_argument_int32(env, (int32_t*)pvalue, val);
    }
    else if (strcmp(s, "ulong") == 0) {
        write_argument_uint32(env, (uint32_t*)pvalue, val);
    }
    else if (strcmp(s, "long") == 0) {
        write_argument_int32(env, (int32_t*)pvalue, val);
    }
    else if (strcmp(s, "longdouble") == 0) {
        write_argument_long_double(env, (long double*)pvalue, val);
    }
    else if (strcmp(s, "pointer") == 0) {
        write_argument_pointer(env, (void**)pvalue, val);
    }
    else if (strcmp(s, "int_p") == 0) {
        Int_write(env, val, (int*)refered);
        *((void**)pvalue) = refered;
    }
    else if (strcmp(s, "pointer_p") == 0) {
        Pointer_write(env, val, (void**)refered);
        *((void**)pvalue) = refered;
    }
    else {
        YogError_raise_ValueError(env, "Unknown argument type");
        /* NOTREACHED */
    }
    RETURN_VOID(env);
}

static uint_t
type2refered_size_of_string(YogEnv* env, YogHandle* type, YogHandle* arg)
{
    uint_t size = YogArray_size(env, HDL2VAL(type));
    if (size < 1) {
        YogError_raise_ValueError(env, "Type array is empty");
        /* NOTREACHED */
    }
    YogVal id = YogArray_at(env, HDL2VAL(type), 0);
    if (!IS_SYMBOL(id)) {
        const char* fmt = "First element of type must be Symbol, not %C";
        YogError_raise_ValueError(env, fmt, id);
        /* NOTREACHED */
    }
    if (VAL2ID(id) != YogVM_intern(env, env->vm, "string")) {
        const char* fmt = "Type must be \'string, not \'%I";
        YogError_raise_ValueError(env, fmt, VAL2ID(id));
    }
    if (size < 2) {
        YogError_raise_ValueError(env, "\'string type needs encoding");
        /* NOTREACHED */
    }
    YogVal enc = YogArray_at(env, HDL2VAL(type), 1);
    if (!IS_PTR(enc) || (BASIC_OBJ_TYPE(enc) != TYPE_ENCODING)) {
        const char* fmt = "Second element of type must be Encoding, not %C";
        YogError_raise_TypeError(env, fmt, enc);
        /* NOTREACHED */
    }
    YogVal h = HDL2VAL(arg);
    if (!IS_PTR(h) || (BASIC_OBJ_TYPE(h) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "Argument must be String, not %C", h);
        /* NOTREACHED */
    }
    return PTR_AS(YogEncoding, enc)->max_size * STRING_SIZE(h);
}

static uint_t
type2refered_size(YogEnv* env, YogVal type, YogVal arg)
{
    if (IS_PTR(type) && (BASIC_OBJ_TYPE(type) == TYPE_ARRAY)) {
        YogHandle* h_type = VAL2HDL(env, type);
        YogHandle* h_arg = VAL2HDL(env, arg);
        return type2refered_size_of_string(env, h_type, h_arg);
    }
    if (!IS_SYMBOL(type)) {
        return 0;
    }
    const char* s = BINARY_CSTR(YogVM_id2bin(env, env->vm, VAL2ID(type)));
    if (strcmp(s, "int_p") == 0) {
        return sizeof(int);
    }
    if (strcmp(s, "pointer_p") == 0) {
        return sizeof(void*);
    }

    return 0;
}

static void
read_argument_pointer(YogEnv* env, YogVal obj, void* ptr)
{
    SAVE_ARG(env, obj);
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_POINTER)) {
        YogError_raise_TypeError(env, "Argument must be Pointer, not %C", obj);
    }
    PTR_AS(Pointer, obj)->ptr = ptr;
    RETURN_VOID(env);
}

static void
read_argument_int(YogEnv* env, YogVal obj, int n)
{
    SAVE_ARG(env, obj);
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_INT)) {
        YogError_raise_TypeError(env, "Argument must be Int, not %C", obj);
    }
    PTR_AS(Int, obj)->value = n;
    RETURN_VOID(env);
}

static void
read_argument(YogEnv* env, YogVal obj, YogVal arg_type, void* p)
{
    if ((p == NULL) || (arg_type == env->vm->cString)) {
        return;
    }

    SAVE_ARGS2(env, obj, arg_type);
    if (!IS_SYMBOL(arg_type)) {
        RETURN_VOID(env);
    }
    YogVal s = YogVM_id2bin(env, env->vm, VAL2ID(arg_type));
    if (strcmp(BINARY_CSTR(s), "int_p") == 0) {
        read_argument_int(env, obj, *((int*)p));
        RETURN_VOID(env);
    }
    if (strcmp(BINARY_CSTR(s), "pointer_p") == 0) {
        read_argument_pointer(env, obj, *((void**)p));
        RETURN_VOID(env);
    }
    YogError_raise_ValueError(env, "Unknown type - %I", VAL2ID(arg_type));
    RETURN_VOID(env);
}

static YogVal
Pointer_new(YogEnv* env, void* ptr)
{
    SAVE_LOCALS(env);
    YogVal p = YUNDEF;
    PUSH_LOCAL(env, p);

    p = Pointer_alloc(env, env->vm->cPointer);
    PTR_AS(Pointer, p)->ptr = ptr;

    RETURN(env, p);
}

static YogVal
LibFunc_do(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    uint_t nargs = HDL_AS(LibFunc, callee)->nargs;
    if (posargc != nargs) {
        const char* fmt = "%u positional argument(s) required, not %u";
        YogError_raise_ValueError(env, fmt, nargs, posargc);
        /* NOTREACHED */
    }
    void** values = (void**)YogSysdeps_alloca(sizeof(void*) * nargs);
    void** refereds = (void**)YogSysdeps_alloca(sizeof(void*) * nargs);
    ffi_type** arg_types = HDL_AS(LibFunc, callee)->cif.arg_types;
    uint_t i;
    for (i = 0; i < nargs; i++) {
        ffi_type* ffi_arg_type = arg_types[i];
        void* pvalue = YogSysdeps_alloca(type2size(env, ffi_arg_type));
        YogVal arg_type = HDL_AS(LibFunc, callee)->arg_types[i];
        uint_t refered_size = type2refered_size(env, arg_type, posargs[i]->val);
        void* refered = 0 < refered_size ? YogSysdeps_alloca(refered_size): NULL;
        write_argument(env, pvalue, refered, arg_type, posargs[i]->val);
        values[i] = pvalue;
        refereds[i] = refered;
    }

    ffi_type* rtype = HDL_AS(LibFunc, callee)->cif.rtype;
    void* rvalue = rtype != &ffi_type_void ? YogSysdeps_alloca(type2size(env, rtype)) : NULL;

    ffi_call(&HDL_AS(LibFunc, callee)->cif, HDL_AS(LibFunc, callee)->f, rvalue, values);

    for (i = 0; i < nargs; i++) {
        YogVal arg_type = HDL_AS(LibFunc, callee)->arg_types[i];
        read_argument(env, posargs[i]->val, arg_type, refereds[i]);
    }

    if (rtype == &ffi_type_uint8) {
        return YogVal_from_unsigned_int(env, *((uint8_t*)rvalue));
    }
    if (rtype == &ffi_type_sint8) {
        return YogVal_from_int(env, *((int8_t*)rvalue));
    }
    if (rtype == &ffi_type_uint16) {
        return YogVal_from_unsigned_int(env, *((uint16_t*)rvalue));
    }
    if (rtype == &ffi_type_sint16) {
        return YogVal_from_int(env, *((int16_t*)rvalue));
    }
    if (rtype == &ffi_type_uint32) {
        return YogVal_from_unsigned_int(env, *((uint32_t*)rvalue));
    }
    if (rtype == &ffi_type_sint32) {
        return YogVal_from_int(env, *((int32_t*)rvalue));
    }
    if (rtype == &ffi_type_uint64) {
        return YogVal_from_unsigned_long_long(env, *((uint64_t*)rvalue));
    }
    if (rtype == &ffi_type_sint64) {
        return YogVal_from_long_long(env, *((int64_t*)rvalue));
    }
    if (rtype == &ffi_type_float) {
        return YogFloat_from_float(env, *((float*)rvalue));
    }
    if (rtype == &ffi_type_double) {
        return YogFloat_from_float(env, *((double*)rvalue));
    }
    if (rtype == &ffi_type_uchar) {
        return YogVal_from_unsigned_int(env, *((unsigned char*)rvalue));
    }
    if (rtype == &ffi_type_schar) {
        return YogVal_from_int(env, *((signed char*)rvalue));
    }
    if (rtype == &ffi_type_ushort) {
        return YogVal_from_unsigned_int(env, *((unsigned short*)rvalue));
    }
    if (rtype == &ffi_type_sshort) {
        return YogVal_from_int(env, *((short*)rvalue));
    }
    if (rtype == &ffi_type_uint) {
        return YogVal_from_unsigned_int(env, *((unsigned int*)rvalue));
    }
    if (rtype == &ffi_type_sint) {
        return YogVal_from_int(env, *((int*)rvalue));
    }
    if (rtype == &ffi_type_ulong) {
        return YogVal_from_unsigned_int(env, *((unsigned long*)rvalue));
    }
    if (rtype == &ffi_type_slong) {
        return YogVal_from_int(env, *((long*)rvalue));
    }
    if (rtype == &ffi_type_longdouble) {
        return YogFloat_from_float(env, *((long double*)rvalue));
    }
    if (rtype == &ffi_type_pointer) {
        return Pointer_new(env, *((void**)rvalue));
    }

    return YNIL;
}

static YogVal
LibFunc_call(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    return LibFunc_do(env, callee, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static void
LibFunc_exec(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogVal retval = LibFunc_do(env, callee, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
    YogEval_push_returned_value(env, env->frame, retval);
}

static YogVal
Struct_get_Buffer(YogEnv* env, YogVal self, YogVal field)
{
    SAVE_ARGS2(env, self, field);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "Object must be Struct, not %C", self);
    }
    if (!IS_PTR(field) || (BASIC_OBJ_TYPE(field) != TYPE_BUFFER_FIELD)) {
        YogError_raise_TypeError(env, "Attribute must be BufferField, not %C", field);
    }

    uint_t index = PTR_AS(BufferField, field)->buffer_index;
    RETURN(env, PTR_AS(Struct, self)->buffers[index]);
}

static YogVal
Struct_get_String(YogEnv* env, YogVal self, YogVal field)
{
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "self must be Struct, not %C", self);
    }
    if (!IS_PTR(field) || (BASIC_OBJ_TYPE(field) != TYPE_STRING_FIELD)) {
        YogError_raise_TypeError(env, "Attribute must be StringField, not %C", field);
    }

    void* ptr = PTR_AS(Struct, self)->data + PTR_AS(FieldBase, field)->offset;
    char* begin = *((char**)ptr);
    if (begin == NULL) {
        return YNIL;
    }
    char* end = begin + strlen(begin);
    YogVal enc = PTR_AS(StringField, field)->encoding;
    return YogEncoding_conv_to_yog(env, VAL2HDL(env, enc), begin, end);
}

static YogVal
Struct_read(YogEnv* env, YogVal self, uint_t offset, ID type)
{
    void* ptr = PTR_AS(Struct, self)->data + offset;
    YogVal val;
    const char* s = BINARY_CSTR(YogVM_id2bin(env, env->vm, type));
    if (strcmp(s, "uint8") == 0) {
        val = INT2VAL(*((uint8_t*)ptr));
    }
    else if (strcmp(s, "int8") == 0) {
        val = INT2VAL(*((int8_t*)ptr));
    }
    else if (strcmp(s, "uint16") == 0) {
        val = INT2VAL(*((uint16_t*)ptr));
    }
    else if (strcmp(s, "int16") == 0) {
        val = INT2VAL(*((int16_t*)ptr));
    }
    else if (strcmp(s, "uint32") == 0) {
        val = YogVal_from_unsigned_int(env, *((uint32_t*)ptr));
    }
    else if (strcmp(s, "int32") == 0) {
        val = YogVal_from_int(env, *((int32_t*)ptr));
    }
    else if (strcmp(s, "uint64") == 0) {
        val = YogVal_from_unsigned_long_long(env, *((uint64_t*)ptr));
    }
    else if (strcmp(s, "int64") == 0) {
        val = YogVal_from_long_long(env, *((int64_t*)ptr));
    }
    else if (strcmp(s, "float") == 0) {
        val = YogFloat_from_float(env, *((float*)ptr));
    }
    else if (strcmp(s, "double") == 0) {
        val = YogFloat_from_float(env, *((double*)ptr));
    }
    else if (strcmp(s, "uchar") == 0) {
        val = INT2VAL(*((uint8_t*)ptr));
    }
    else if (strcmp(s, "char") == 0) {
        val = INT2VAL(*((int8_t*)ptr));
    }
    else if (strcmp(s, "ushort") == 0) {
        val = INT2VAL(*((uint16_t*)ptr));
    }
    else if (strcmp(s, "short") == 0) {
        val = INT2VAL(*((int16_t*)ptr));
    }
    else if (strcmp(s, "uint") == 0) {
        val = YogVal_from_unsigned_int(env, *((uint32_t*)ptr));
    }
    else if (strcmp(s, "int") == 0) {
        val = YogVal_from_int(env, *((int32_t*)ptr));
    }
    else if (strcmp(s, "ulong") == 0) {
        val = YogVal_from_unsigned_int(env, *((uint32_t*)ptr));
    }
    else if (strcmp(s, "long") == 0) {
        val = YogVal_from_int(env, *((int32_t*)ptr));
    }
    else if (strcmp(s, "longdouble") == 0) {
        val = YogFloat_from_float(env, *((long double*)ptr));
    }
    else if (strcmp(s, "pointer") == 0) {
        val = Pointer_new(env, *((void**)ptr));
    }
    else {
        const char* fmt = "Unknown type for reading struct member - %I";
        YogError_raise_ValueError(env, fmt, type);
        /* NOTREACHED */
        /**
         * gcc complains "‘val’ may be used uninitialized in this function"
         * without the following line.
         */
        val = YUNDEF;
    }

    return val;
}

static YogVal
Struct_get(YogEnv* env, YogVal self, YogVal field)
{
    SAVE_ARGS2(env, self, field);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "self must be Struct, not %C", self);
    }
    if (!IS_PTR(field) || (BASIC_OBJ_TYPE(field) != TYPE_FIELD)) {
        YogError_raise_TypeError(env, "Attribute must be Field, not %C", field);
    }

    uint_t offset = PTR_AS(FieldBase, field)->offset;
    val = Struct_read(env, self, offset, PTR_AS(Field, field)->type);
    RETURN(env, val);
}

static YogVal
FieldArray_new(YogEnv* env, YogVal st, YogVal field)
{
    SAVE_ARGS2(env, st, field);
    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);
    YOG_ASSERT(env, IS_PTR(st) && (BASIC_OBJ_TYPE(st) == TYPE_STRUCT), "Invalid structure (%p)", BASIC_OBJ_TYPE(st));

    array = FieldArray_alloc(env, env->vm->cFieldArray);
    PTR_AS(FieldArray, array)->st = st;
    PTR_AS(FieldArray, array)->field = field;

    RETURN(env, array);
}

static void
ArrayField_exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    array = FieldArray_new(env, obj, attr);
    YogScriptFrame_push_stack(env, env->frame, array);

    RETURN_VOID(env);
}

static YogVal
BufferField_call_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);
    val = Struct_get_Buffer(env, obj, attr);
    RETURN(env, val);
}

static void
BufferField_exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    val = Struct_get_Buffer(env, obj, attr);
    YogScriptFrame_push_stack(env, env->frame, val);

    RETURN_VOID(env);
}

static YogVal
StringField_call_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);
    val = Struct_get_String(env, obj, attr);
    RETURN(env, val);
}

static void
StringField_exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    val = Struct_get_String(env, obj, attr);
    YogScriptFrame_push_stack(env, env->frame, val);

    RETURN_VOID(env);
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
ArrayField_call_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);
    array = FieldArray_new(env, obj, attr);
    RETURN(env, array);
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

#define STRUCT_WRITE_DATA(type, obj, offset, val) do { \
    *((type*)(PTR_AS(Struct, (obj))->data + (offset))) = (val); \
} while (0)

static void
Struct_write_uint8(YogEnv* env, YogVal self, uint_t offset, YogVal val)
{
    SAVE_ARGS2(env, self, val);

    check_Fixnum_uint8(env, val);
    STRUCT_WRITE_DATA(uint8_t, self, offset, VAL2INT(val));

    RETURN_VOID(env);
}

static void
Struct_write_int8(YogEnv* env, YogVal self, uint_t offset, YogVal val)
{
    SAVE_ARGS2(env, self, val);

    check_Fixnum_int8(env, val);
    STRUCT_WRITE_DATA(int8_t, self, offset, VAL2INT(val));

    RETURN_VOID(env);
}

static void
Struct_write_uint16(YogEnv* env, YogVal self, uint_t offset, YogVal val)
{
    SAVE_ARGS2(env, self, val);

    check_Fixnum_uint16(env, val);
    STRUCT_WRITE_DATA(uint16_t, self, offset, VAL2INT(val));

    RETURN_VOID(env);
}

static void
Struct_write_int16(YogEnv* env, YogVal self, uint_t offset, YogVal val)
{
    SAVE_ARGS2(env, self, val);

    check_Fixnum_int16(env, val);
    STRUCT_WRITE_DATA(int16_t, self, offset, VAL2INT(val));

    RETURN_VOID(env);
}

#define WRITE_POSITIVE_NUM(env, type, obj, offset, val) do { \
    if (VAL2INT((val)) < 0) { \
        YogError_raise_ValueError((env), "Value must be greater or equal 0, not %d", VAL2INT((val))); \
    } \
    STRUCT_WRITE_DATA(type, (obj), (offset), VAL2INT((val))); \
} while (0)

static void
Struct_write_uint32(YogEnv* env, YogVal self, uint_t offset, YogVal val)
{
    SAVE_ARGS2(env, self, val);

    if (IS_FIXNUM(val)) {
        WRITE_POSITIVE_NUM(env, uint32_t, self, offset, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_uint32(env, val);
        uint_t n = YogBignum_to_unsigned_type(env, val, "Value");
        STRUCT_WRITE_DATA(uint32_t, self, offset, n);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
Struct_write_int32(YogEnv* env, YogVal self, uint_t offset, YogVal val)
{
    SAVE_ARGS2(env, self, val);

    if (IS_FIXNUM(val)) {
        STRUCT_WRITE_DATA(int32_t, self, offset, VAL2INT(val));
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_int32(env, val);
        int_t n = YogBignum_to_signed_type(env, val, "Value");
        STRUCT_WRITE_DATA(int32_t, self, offset, n);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
Struct_write_uint64(YogEnv* env, YogVal self, uint_t offset, YogVal val)
{
    SAVE_ARGS2(env, self, val);

    if (IS_FIXNUM(val)) {
        WRITE_POSITIVE_NUM(env, uint64_t, self, offset, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_uint64(env, val);
        unsigned long long n = YogBignum_to_unsigned_long_long(env, val, "Value");
        STRUCT_WRITE_DATA(uint64_t, self, offset, n);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
Struct_write_int64(YogEnv* env, YogVal self, uint_t offset, YogVal val)
{
    SAVE_ARGS2(env, self, val);

    if (IS_FIXNUM(val)) {
        STRUCT_WRITE_DATA(int64_t, self, offset, VAL2INT(val));
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_int64(env, val);
        long long n = YogBignum_to_long_long(env, val, "Value");
        STRUCT_WRITE_DATA(int64_t, self, offset, n);
        RETURN_VOID(env);
    }
    YogError_raise_TypeError(env, "Value must be Fixnum or Bignum, not %C", val);

    RETURN_VOID(env);
}

static void
Struct_write_Buffer(YogEnv* env, YogVal self, YogVal field, YogVal buf)
{
    SAVE_ARGS3(env, self, field, buf);
    uint_t index = PTR_AS(BufferField, field)->buffer_index;
    YogGC_UPDATE_PTR(env, PTR_AS(Struct, self), buffers[index], buf);
    uint_t offset = PTR_AS(FieldBase, field)->offset;
    STRUCT_WRITE_DATA(void*, self, offset, PTR_AS(Buffer, buf)->ptr);
    RETURN_VOID(env);
}

static void
StructField_exec_descr_set(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    SAVE_ARGS3(env, attr, obj, val);
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "Value must be Struct, not %C", val);
    }
    YogVal klass = PTR_AS(StructField, attr)->klass;
    if (klass != PTR_AS(YogBasicObj, val)->klass) {
        ID name = PTR_AS(YogClass, klass)->name;
        YogError_raise_TypeError(env, "Value must be %I, not %C", name, val);
    }
    uint_t offset = PTR_AS(FieldBase, attr)->offset;
    void* dest = (char*)PTR_AS(Struct, obj)->data + offset;
    memcpy(dest, PTR_AS(Struct, val)->data, PTR_AS(StructClass, klass)->size);
    RETURN_VOID(env);
}

static void
ArrayField_exec_descr_set(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    SAVE_ARGS3(env, attr, obj, val);
    YogError_raise_FFIError(env, "Array field is not writable.");
    /* NOTREACHED */
    RETURN_VOID(env);
}

static void
BufferField_exec_descr_set(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    SAVE_ARGS3(env, attr, obj, val);
    if (!IS_PTR(attr) || (BASIC_OBJ_TYPE(attr) != TYPE_BUFFER_FIELD)) {
        YogError_raise_TypeError(env, "Attribute must be Field, not %C", attr);
    }
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "Object must be Struct, not %C", obj);
    }
    Struct_write_Buffer(env, obj, attr, val);
    RETURN_VOID(env);
}

static void
StringField_exec_descr_set(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    YogError_raise_FFIError(env, "\'string field is not writable.");
    /* NOTREACHED */
}

static void
Struct_write(YogEnv* env, YogVal self, uint_t offset, ID type, YogVal val)
{
    SAVE_ARGS2(env, self, val);
#define WRITE_FLOAT(type) do { \
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_FLOAT)) { \
        YogError_raise_TypeError(env, "Value must be Float, not %C", val); \
    } \
    STRUCT_WRITE_DATA(type, self, offset, FLOAT_NUM(val)); \
} while (0)
    const char* s = BINARY_CSTR(YogVM_id2bin(env, env->vm, type));
    if (strcmp(s, "uint8") == 0) {
        Struct_write_uint8(env, self, offset, val);
    }
    else if (strcmp(s, "int8") == 0) {
        Struct_write_int8(env, self, offset, val);
    }
    else if (strcmp(s, "uint16") == 0) {
        Struct_write_uint16(env, self, offset, val);
    }
    else if (strcmp(s, "int16") == 0) {
        Struct_write_int16(env, self, offset, val);
    }
    else if (strcmp(s, "uint32") == 0) {
        Struct_write_uint32(env, self, offset, val);
    }
    else if (strcmp(s, "int32") == 0) {
        Struct_write_int32(env, self, offset, val);
    }
    else if (strcmp(s, "uint64") == 0) {
        Struct_write_uint64(env, self, offset, val);
    }
    else if (strcmp(s, "int64") == 0) {
        Struct_write_int64(env, self, offset, val);
    }
    else if (strcmp(s, "float") == 0) {
        WRITE_FLOAT(float);
    }
    else if (strcmp(s, "double") == 0) {
        WRITE_FLOAT(double);
    }
    else if (strcmp(s, "uchar") == 0) {
        Struct_write_uint8(env, self, offset, val);
    }
    else if (strcmp(s, "char") == 0) {
        Struct_write_int8(env, self, offset, val);
    }
    else if (strcmp(s, "ushort") == 0) {
        Struct_write_uint16(env, self, offset, val);
    }
    else if (strcmp(s, "short") == 0) {
        Struct_write_int16(env, self, offset, val);
    }
    else if (strcmp(s, "uint") == 0) {
        Struct_write_uint32(env, self, offset, val);
    }
    else if (strcmp(s, "int") == 0) {
        Struct_write_int32(env, self, offset, val);
    }
    else if (strcmp(s, "ulong") == 0) {
        Struct_write_uint32(env, self, offset, val);
    }
    else if (strcmp(s, "long") == 0) {
        Struct_write_int32(env, self, offset, val);
    }
    else if (strcmp(s, "longdouble") == 0) {
        WRITE_FLOAT(long double);
    }
    else if (strcmp(s, "pointer") == 0) {
        Struct_write_uint32(env, self, offset, val);
    }
    else {
        YogError_raise_ValueError(env, "unknown type - %I", type);
        /* NOTREACHED */
    }
#undef WRITE_FLOAT
    RETURN_VOID(env);
}

static void
Field_exec_descr_set(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    SAVE_ARGS3(env, attr, obj, val);
    if (!IS_PTR(attr) || (BASIC_OBJ_TYPE(attr) != TYPE_FIELD)) {
        YogError_raise_TypeError(env, "Attribute must be Field, not %C", attr);
    }
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "Object must be Struct, not %C", obj);
    }

    uint_t offset = PTR_AS(FieldBase, attr)->offset;
    Struct_write(env, obj, offset, PTR_AS(Field, attr)->type, val);

    RETURN_VOID(env);
}

static void
Buffer_alloc_buffer(YogEnv* env, YogVal self, int_t size)
{
    SAVE_ARG(env, self);
    if (size < 0) {
        YogError_raise_ValueError(env, "size must be greater than or equal to zero, not %d", size);
    }

    void* ptr = YogGC_malloc(env, size);
    PTR_AS(Buffer, self)->size = size;
    PTR_AS(Buffer, self)->ptr = ptr;
    RETURN_VOID(env);
}

static void
Buffer_alloc_binary(YogEnv* env, YogVal self, YogVal bin)
{
    SAVE_ARGS2(env, self, bin);

    uint_t size = YogBinary_size(env, bin);
    void* ptr = YogGC_malloc(env, size);
    memcpy(ptr, BINARY_CSTR(bin), size);
    PTR_AS(Buffer, self)->size = size;
    PTR_AS(Buffer, self)->ptr = ptr;
    RETURN_VOID(env);
}

static void
Buffer_alloc_string(YogEnv* env, YogVal self, YogVal s)
{
    SAVE_ARGS2(env, self, s);

    uint_t size = sizeof(YogChar) * STRING_SIZE(s);
    void* ptr = YogGC_malloc(env, size);
    memcpy(ptr, STRING_CHARS(s), size);
    PTR_AS(Buffer, self)->size = size;
    PTR_AS(Buffer, self)->ptr = ptr;
    RETURN_VOID(env);
}

static YogVal
Buffer_to_bin(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal bin = YUNDEF;
    YogVal size = YUNDEF;
    PUSH_LOCALS2(env, bin, size);
    YogCArg params[] = { { "size", &size }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_bin", params, args, kw);
    CHECK_SELF_BUFFER;
    if (!IS_FIXNUM(size)) {
        YogError_raise_TypeError(env, "size must be Fixnum, not %C", size);
    }

    bin = YogBinary_of_size(env, VAL2INT(size));
    memcpy(BINARY_CSTR(bin), PTR_AS(Buffer, self)->ptr, VAL2INT(size));
    BINARY_SIZE(bin) = VAL2INT(size);
    RETURN(env, bin);
}

static YogVal
Pointer_to_s(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_s", params, args, kw);
    CHECK_SELF_POINTER;
    void* p = PTR_AS(Pointer, self)->ptr;
    RETURN(env, YogSprintf_sprintf(env, "<Pointer %p>", p));
}

static YogVal
Buffer_to_s(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    YogVal enc = YUNDEF;
    YogVal size = YUNDEF;
    PUSH_LOCALS3(env, s, enc, size);
    YogCArg params[] = {
        { "size", &size },
        { "encoding", &enc },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_s", params, args, kw);
    CHECK_SELF_BUFFER;
    if (!IS_FIXNUM(size)) {
        YogError_raise_TypeError(env, "size must be Fixnum, not %C", size);
    }
    if ((VAL2INT(size) < 0) || (PTR_AS(Buffer, self)->size < VAL2INT(size))) {
        YogError_raise_ValueError(env, "Out of Buffer size - %d", VAL2INT(size));
    }

    if (VAL2INT(size) == 0) {
        RETURN(env, YogString_new(env));
    }
    const char* begin = PTR_AS(Buffer, self)->ptr;
    const char* end = begin + VAL2INT(size);
    s = YogEncoding_conv_to_yog(env, VAL2HDL(env, enc), begin, end);

    RETURN(env, s);
}

static int_t
FieldArray_normalize_index(YogEnv* env, YogVal self, int_t index)
{
    return index < 0 ? index + PTR_AS(ArrayField, self)->size : index;
}

static YogVal
FieldArray_subscript_assign(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal val = YUNDEF;
    YogVal index = YUNDEF;
    YogVal field = YUNDEF;
    PUSH_LOCALS3(env, val, index, field);
    YogCArg params[] = {
        { "index", &index },
        { "value", &val },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]=", params, args, kw);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_FIELD_ARRAY)) {
        YogError_raise_TypeError(env, "self must be FieldArray, not %C", self);
    }
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "index must be Fixnum, not %C", index);
    }

    field = PTR_AS(FieldArray, self)->field;
    YOG_ASSERT(env, BASIC_OBJ_TYPE(field) == TYPE_ARRAY_FIELD, "Invalid field");
    int_t size = PTR_AS(ArrayField, field)->size;
    int_t idx = FieldArray_normalize_index(env, field, VAL2INT(index));
    if ((idx < 0) || ((size != 0) && (size <= idx))) {
        YogError_raise_IndexError(env, "Index out of range - %d", VAL2INT(index));
    }
    ID type = PTR_AS(ArrayField, field)->type;
    uint_t offset = PTR_AS(FieldBase, field)->offset + id2size(env, type) * idx;
    Struct_write(env, PTR_AS(FieldArray, self)->st, offset, type, val);
    RETURN(env, self);
}

static YogVal
FieldArray_to_s(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal field = YUNDEF;
    YogVal st = YUNDEF;
    PUSH_LOCALS2(env, field, st);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_s", params, args, kw);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_FIELD_ARRAY)) {
        YogError_raise_TypeError(env, "self must be FieldArray, not %C", self);
    }
    field = PTR_AS(FieldArray, self)->field;
    ID id_char = YogVM_intern(env, env->vm, "char");
    if (PTR_AS(ArrayField, field)->type == id_char) {
        YogHandle* enc = VAL2HDL(env, YogEncoding_get_ascii(env));
        st = PTR_AS(FieldArray, self)->st;
        uint_t offset = PTR_AS(FieldBase, field)->offset;
        const char* begin = PTR_AS(Struct, st)->data + offset;
        const char* end = begin + PTR_AS(ArrayField, field)->size;
        RETURN(env, YogEncoding_conv_to_yog(env, enc, begin, end));
    }
    RETURN(env, YogBasicObj_to_s(env, self));
}

static YogVal
FieldArray_subscript(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal val = YUNDEF;
    YogVal index = YUNDEF;
    YogVal field = YUNDEF;
    PUSH_LOCALS3(env, val, index, field);
    YogCArg params[] = { { "index", &index }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]", params, args, kw);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_FIELD_ARRAY)) {
        YogError_raise_TypeError(env, "self must be FieldArray, not %C", self);
    }
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "index must be Fixnum, not %C", index);
    }

    field = PTR_AS(FieldArray, self)->field;
    YOG_ASSERT(env, BASIC_OBJ_TYPE(field) == TYPE_ARRAY_FIELD, "Invalid field");
    int_t size = PTR_AS(ArrayField, field)->size;
    int_t idx = FieldArray_normalize_index(env, field, VAL2INT(index));
    if ((idx < 0) || ((size != 0) && (size <= idx))) {
        YogError_raise_IndexError(env, "Index out of range - %d", VAL2INT(index));
    }
    ID type = PTR_AS(ArrayField, field)->type;
    uint_t offset = PTR_AS(FieldBase, field)->offset + id2size(env, type) * idx;
    val = Struct_read(env, PTR_AS(FieldArray, self)->st, offset, type);
    RETURN(env, val);
}

static YogVal
Buffer_init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal data = YUNDEF;
    PUSH_LOCAL(env, data);
    YogCArg params[] = { { "data", &data }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);
#define RAISE_TYPE_ERROR do { \
    YogError_raise_TypeError(env, "data must be String, Binary or Fixnum, not %C", data); \
} while (0)
    if (IS_FIXNUM(data)) {
        Buffer_alloc_buffer(env, self, VAL2INT(data));
        RETURN(env, self);
    }
    if (!IS_PTR(data)) {
        RAISE_TYPE_ERROR;
    }
    if (BASIC_OBJ_TYPE(data) == TYPE_STRING) {
        Buffer_alloc_string(env, self, data);
        RETURN(env, self);
    }
    if (BASIC_OBJ_TYPE(data) == TYPE_BINARY) {
        Buffer_alloc_binary(env, self, data);
        RETURN(env, self);
    }
    RAISE_TYPE_ERROR;
#undef RAISE_TYPE_ERROR

    RETURN(env, self);
}

static YogVal
Buffer_get_size(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal size = YUNDEF;
    PUSH_LOCAL(env, size);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_size", params, args, kw);
    CHECK_SELF_BUFFER;

    size = YogVal_from_unsigned_int(env, PTR_AS(Buffer, self)->size);

    RETURN(env, size);
}

static YogVal
Int_get_value(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal value = YUNDEF;
    PUSH_LOCAL(env, value);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_value", params, args, kw);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_INT)) {
        YogError_raise_TypeError(env, "self must be Int, not %C", self);
    }

    value = YogVal_from_int(env, PTR_AS(Int, self)->value);

    RETURN(env, value);
}

static YogVal
StructField_call_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal st = StructBase_alloc(env, PTR_AS(StructField, attr)->klass);
    uint_t offset = PTR_AS(FieldBase, attr)->offset;
    PTR_AS(Struct, st)->data = (char*)PTR_AS(Struct, obj)->data + offset;
    PTR_AS(Struct, st)->own = FALSE;
    YogGC_UPDATE_PTR(env, PTR_AS(Struct, st), top, PTR_AS(Struct, obj)->top);
    RETURN(env, st);
}

static void
StructField_exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    YogVal st = StructField_call_descr_get(env, attr, obj, klass);
    YogScriptFrame_push_stack(env, env->frame, st);
}

void
YogFFI_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cArrayField = YUNDEF;
    YogVal cBuffer = YUNDEF;
    YogVal cBufferField = YUNDEF;
    YogVal cField = YUNDEF;
    YogVal cFieldArray = YUNDEF;
    YogVal cInt = YUNDEF;
    YogVal cLib = YUNDEF;
    YogVal cLibFunc = YUNDEF;
    YogVal cPointer = YUNDEF;
    YogVal cStringField = YUNDEF;
    YogVal cStructBase = YUNDEF;
    YogVal cStructClass = YUNDEF;
    YogVal cStructClassClass = YUNDEF;
    YogVal cStructField = YUNDEF;
    PUSH_LOCALS8(env, cLib, cLibFunc, cStructClassClass, cStructClass, cField, cInt, cBuffer, cPointer);
    PUSH_LOCALS6(env, cArrayField, cBufferField, cStringField, cFieldArray, cStructField, cStructBase);
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
    cStructBase = YogClass_new(env, "StructBase", vm->cClass);
    YogClass_define_property(env, cStructBase, pkg, "size", StructBase_get_size, NULL);
    YogClass_define_allocator(env, cStructBase, StructBase_alloc);
    YogClass_define_method(env, cStructBase, pkg, "init", StructBase_init);
    vm->cStructBase = cStructBase;
    cStructClass = YogClass_new(env, "StructClass", vm->cClass);
    YogGC_UPDATE_PTR(env, PTR_AS(YogBasicObj, cStructClass), klass, cStructClassClass);
    vm->cStructClass = cStructClass;
    cField = YogClass_new(env, "Field", vm->cObject);
    YogClass_define_allocator(env, cField, Field_alloc);
    YogClass_define_descr_get_executor(env, cField, Field_exec_descr_get);
    YogClass_define_descr_get_caller(env, cField, Field_call_descr_get);
    YogClass_define_descr_set_executor(env, cField, Field_exec_descr_set);
    vm->cField = cField;
    cStructField = YogClass_new(env, "StructField", vm->cObject);
    YogClass_define_allocator(env, cStructField, StructField_alloc);
    YogClass_define_descr_get_executor(env, cStructField, StructField_exec_descr_get);
    YogClass_define_descr_get_caller(env, cStructField, StructField_call_descr_get);
    YogClass_define_descr_set_executor(env, cStructField, StructField_exec_descr_set);
    vm->cStructField = cStructField;
    cArrayField = YogClass_new(env, "ArrayField", vm->cObject);
    YogClass_define_allocator(env, cArrayField, ArrayField_alloc);
    YogClass_define_descr_get_executor(env, cArrayField, ArrayField_exec_descr_get);
    YogClass_define_descr_get_caller(env, cArrayField, ArrayField_call_descr_get);
    YogClass_define_descr_set_executor(env, cArrayField, ArrayField_exec_descr_set);
    vm->cArrayField = cArrayField;
    cBufferField = YogClass_new(env, "BufferField", vm->cObject);
    YogClass_define_allocator(env, cBufferField, BufferField_alloc);
    YogClass_define_descr_get_executor(env, cBufferField, BufferField_exec_descr_get);
    YogClass_define_descr_get_caller(env, cBufferField, BufferField_call_descr_get);
    YogClass_define_descr_set_executor(env, cBufferField, BufferField_exec_descr_set);
    vm->cBufferField = cBufferField;
    cStringField = YogClass_new(env, "StringField", vm->cObject);
    YogClass_define_allocator(env, cStringField, StringField_alloc);
    YogClass_define_descr_get_executor(env, cStringField, StringField_exec_descr_get);
    YogClass_define_descr_get_caller(env, cStringField, StringField_call_descr_get);
    YogClass_define_descr_set_executor(env, cStringField, StringField_exec_descr_set);
    vm->cStringField = cStringField;
    cFieldArray = YogClass_new(env, "FieldArray", vm->cObject);
    YogClass_define_allocator(env, cFieldArray, FieldArray_alloc);
    YogClass_define_method(env, cFieldArray, pkg, "[]", FieldArray_subscript);
    YogClass_define_method(env, cFieldArray, pkg, "[]=", FieldArray_subscript_assign);
    YogClass_define_method(env, cFieldArray, pkg, "to_s", FieldArray_to_s);
    vm->cFieldArray = cFieldArray;

    cInt = YogClass_new(env, "Int", vm->cObject);
    YogClass_define_allocator(env, cInt, Int_alloc);
    YogClass_define_property(env, cInt, pkg, "value", Int_get_value, NULL);
    vm->cInt = cInt;
    cBuffer = YogClass_new(env, "Buffer", vm->cObject);
    YogClass_define_allocator(env, cBuffer, Buffer_alloc);
    YogClass_define_method(env, cBuffer, pkg, "init", Buffer_init);
    YogClass_define_method(env, cBuffer, pkg, "to_bin", Buffer_to_bin);
    YogClass_define_method(env, cBuffer, pkg, "to_s", Buffer_to_s);
    YogClass_define_property(env, cBuffer, pkg, "size", Buffer_get_size, NULL);
    vm->cBuffer = cBuffer;
    cPointer = YogClass_new(env, "Pointer", vm->cObject);
    YogClass_define_allocator(env, cPointer, Pointer_alloc);
    YogClass_define_method(env, cPointer, pkg, "to_s", Pointer_to_s);
    vm->cPointer = cPointer;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
