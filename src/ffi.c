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
    YogVal rtype;
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
    uint_t child_index;
};

typedef struct BufferField BufferField;

#define TYPE_BUFFER_FIELD TO_TYPE(BufferField_alloc)

struct PointerField {
    struct FieldBase base;
    YogVal klass;
    uint_t child_index;
};

typedef struct PointerField PointerField;

#define TYPE_POINTER_FIELD TO_TYPE(PointerField_alloc)

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
    uint_t children_num;
    uint_t alignment_unit;
};

typedef struct StructClass StructClass;

#define TYPE_STRUCT_CLASS TO_TYPE(StructClass_new)
static YogVal StructClass_new(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block);

struct Struct {
    struct YogBasicObj base;
    void* data;
    BOOL own;
    YogVal top;
    YogVal children[0];
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
#define CHECK_SELF_STRUCT do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) { \
        YogError_raise_TypeError(env, "self must be Struct, not %C", self); \
    } \
} while (0)
#define CHECK_STRUCT_OR_UNION_CLASS(env, klass, obj, name) do { \
    if (!IS_PTR((obj)) || (BASIC_OBJ_TYPE((obj)) != TYPE_STRUCT_CLASS)) { \
        const char* fmt = "%s must be %s, not %C"; \
        YogError_raise_TypeError((env), fmt, (name), (klass), (obj)); \
    } \
} while (0)
#define CHECK_STRUCT_CLASS(env, obj, name) do { \
    CHECK_STRUCT_OR_UNION_CLASS((env), "StructClass", (obj), (name)); \
} while (0)
#define CHECK_SELF_STRUCT_CLASS CHECK_STRUCT_CLASS(env, self, "self")
#define CHECK_UNION_CLASS(env, obj, name) do { \
    CHECK_STRUCT_OR_UNION_CLASS((env), "UnionClass", (obj), (name)); \
} while (0)
#define CHECK_SELF_UNION_CLASS CHECK_UNION_CLASS(env, self, "self")

struct Pointer {
    struct YogBasicObj base;
    void* ptr;
};

typedef struct Pointer Pointer;

#define TYPE_POINTER TO_TYPE(Pointer_alloc)

enum NodeType {
    NODE_ARRAY,
    NODE_ATOM,
    NODE_BUFFER,
    NODE_FIELD,
    NODE_POINTER,
    NODE_STRING,
    NODE_STRUCT,
};

typedef enum NodeType NodeType;

struct Node {
    enum NodeType type;
    union {
        struct {
            YogVal type;
            uint_t size;
        } array;
        struct {
            ID type;
        } atom;
        struct {
            YogVal next;
            ID name;
            YogVal type;
        } field;
        struct {
            ID name;
        } name;
        struct {
            YogVal klass;
        } pointer;
        struct {
            YogVal encoding;
        } string;
        struct {
            YogVal klass;
        } struct_;
    } u;
};

typedef struct Node Node;

static void
Node_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    Node* node = (Node*)ptr;
    switch (node->type) {
    case NODE_ARRAY:
        YogGC_KEEP(env, node, u.array.type, keeper, heap);
        break;
    case NODE_FIELD:
        YogGC_KEEP(env, node, u.field.next, keeper, heap);
        YogGC_KEEP(env, node, u.field.type, keeper, heap);
        break;
    case NODE_POINTER:
        YogGC_KEEP(env, node, u.pointer.klass, keeper, heap);
        break;
    case NODE_STRING:
        YogGC_KEEP(env, node, u.string.encoding, keeper, heap);
        break;
    case NODE_STRUCT:
        YogGC_KEEP(env, node, u.struct_.klass, keeper, heap);
        break;
    case NODE_ATOM:
    case NODE_BUFFER:
        break;
    default:
        YOG_BUG(env, "Invalid Node type (%u)", node->type);
    }
}

static YogVal
Node_new(YogEnv* env)
{
    YogVal node = ALLOC_OBJ(env, Node_keep_children, NULL, Node);
    PTR_AS(Node, node)->type = NODE_ARRAY;
    PTR_AS(Node, node)->u.array.type = YUNDEF;
    PTR_AS(Node, node)->u.array.size = 0;
    return node;
}

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

static void
PointerField_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    PointerField* field = (PointerField*)ptr;
    YogGC_KEEP(env, field, klass, keeper, heap);
}

static YogVal
BufferField_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    field = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, BufferField);
    FieldBase_init(env, field, TYPE_BUFFER_FIELD, klass);
    PTR_AS(BufferField, field)->child_index = 0;

    RETURN(env, field);
}

static YogVal
PointerField_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);

    field = ALLOC_OBJ(env, PointerField_keep_children, NULL, PointerField);
    FieldBase_init(env, field, TYPE_POINTER_FIELD, klass);
    PTR_AS(PointerField, field)->klass = YUNDEF;
    PTR_AS(PointerField, field)->child_index = 0;

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
BufferField_new(YogEnv* env, uint_t offset, uint_t child_index)
{
    YogVal field = BufferField_alloc(env, env->vm->cBufferField);
    PTR_AS(FieldBase, field)->offset = offset;
    PTR_AS(BufferField, field)->child_index = child_index;
    return field;
}

static YogVal
PointerField_new(YogEnv* env, uint_t offset, YogVal klass, uint_t child_index)
{
    SAVE_ARG(env, klass);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);
    CHECK_STRUCT_CLASS(env, klass, "pointer field");

    field = PointerField_alloc(env, env->vm->cPointerField);
    PTR_AS(FieldBase, field)->offset = offset;
    YogGC_UPDATE_PTR(env, PTR_AS(PointerField, field), klass, klass);
    PTR_AS(PointerField, field)->child_index = child_index;

    RETURN(env, field);
}

static YogVal
ArrayField_new(YogEnv* env, YogVal node, uint_t offset)
{
    SAVE_ARG(env, node);
    YogVal type = PTR_AS(Node, node)->u.array.type;
    PUSH_LOCAL(env, type);
    if (PTR_AS(Node, type)->type != NODE_ATOM) {
        YogError_raise_TypeError(env, "Type of array must be atom");
    }

    YogVal field = ArrayField_alloc(env, env->vm->cArrayField);
    PTR_AS(FieldBase, field)->offset = offset;
    PTR_AS(ArrayField, field)->type = PTR_AS(Node, type)->u.atom.type;
    PTR_AS(ArrayField, field)->size = PTR_AS(Node, node)->u.array.size;
    RETURN(env, field);
}

static YogVal
create_field(YogEnv* env, YogVal node, uint_t offset, uint_t child_index)
{
    YogVal klass;
    YogVal encoding;
    switch (PTR_AS(Node, node)->type) {
    case NODE_ARRAY:
        return ArrayField_new(env, node, offset);
    case NODE_ATOM:
        return Field_new(env, offset, PTR_AS(Node, node)->u.atom.type);
    case NODE_BUFFER:
        return BufferField_new(env, offset, child_index);
    case NODE_POINTER:
        klass = PTR_AS(Node, node)->u.pointer.klass;
        return PointerField_new(env, offset, klass, child_index);
    case NODE_STRING:
        encoding = PTR_AS(Node, node)->u.string.encoding;
        return StringField_new(env, offset, encoding);
    case NODE_STRUCT:
        klass = PTR_AS(Node, node)->u.struct_.klass;
        return StructField_new(env, offset, klass);
    case NODE_FIELD:
    default:
        YOG_BUG(env, "Invalid Node type (%u)", PTR_AS(Node, node)->type);
    }

    return YUNDEF;
}

static void
StructClass_set_field(YogEnv* env, YogVal self, YogVal node, uint_t offset, uint_t child_index)
{
    SAVE_ARGS2(env, self, node);

    ID name = PTR_AS(Node, node)->u.field.name;
    YogVal type = PTR_AS(Node, node)->u.field.type;
    YogVal field = create_field(env, type, offset, child_index);
    YogObj_set_attr_id(env, self, name, field);

    RETURN_VOID(env);
}

static void
init_struct(YogEnv* env, YogVal obj, YogVal klass)
{
    YogClass_init(env, obj, TYPE_STRUCT_CLASS, klass);
    PTR_AS(StructClass, obj)->size = 0;
    YogGC_UPDATE_PTR(env, PTR_AS(YogClass, obj), super, env->vm->cStructBase);
}

static void
UnionClass_init(YogEnv* env, YogVal self)
{
    init_struct(env, self, env->vm->cUnionClass);
}

static void
StructClass_init(YogEnv* env, YogVal self)
{
    init_struct(env, self, env->vm->cStructClass);
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
    else if (strcmp(s, "ulonglong") == 0) {
        size = sizeof(unsigned long long);
    }
    else if (strcmp(s, "longlong") == 0) {
        size = sizeof(long long);
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
get_size(YogEnv* env, YogVal node)
{
    YogVal klass;
    switch (PTR_AS(Node, node)->type) {
    case NODE_ATOM:
        return id2size(env, PTR_AS(Node, node)->u.atom.type);
    case NODE_BUFFER:
    case NODE_POINTER:
    case NODE_STRING:
        return sizeof(void*);
    case NODE_STRUCT:
        klass = PTR_AS(Node, node)->u.struct_.klass;
        return PTR_AS(StructClass, klass)->size;
    case NODE_ARRAY:
    case NODE_FIELD:
    default:
        YOG_BUG(env, "Invalid Node type (%u)", PTR_AS(Node, node)->type);
    }

    return 0;
}

static YogVal
get_fields_of_anonymous_struct(YogEnv* env, YogVal type)
{
    if (YogArray_size(env, type) < 1) {
        YogError_raise_ValueError(env, "Anonymous struct/union have no fields");
    }
    YogVal fields = YogArray_at(env, type, 1);
    if (!IS_PTR(fields) || (BASIC_OBJ_TYPE(fields) != TYPE_ARRAY)) {
        const char* fmt = "Anonymous struct/union fields must be Array, not %C";
        YogError_raise_TypeError(env, fmt, fields);
    }
    return fields;
}

static uint_t
get_alignment_unit_of_node(YogEnv* env, YogVal node)
{
    YogVal klass;
    switch (PTR_AS(Node, node)->type) {
    case NODE_ATOM:
    case NODE_BUFFER:
    case NODE_POINTER:
    case NODE_STRING:
        return get_size(env, node);
    case NODE_STRUCT:
        klass = PTR_AS(Node, node)->u.struct_.klass;
        return PTR_AS(StructClass, klass)->alignment_unit;
    case NODE_ARRAY:
    case NODE_FIELD:
    default:
        YOG_BUG(env, "Invalid Node type (%u)", PTR_AS(Node, node)->type);
    }

    return 0;
}

static uint_t
get_alignment_unit(YogEnv* env, YogVal nodes)
{
    SAVE_ARG(env, nodes);
    if (!IS_PTR(nodes)) {
        RETURN(env, 0);
    }

    YogVal node = PTR_AS(Node, nodes)->u.field.type;
    if (PTR_AS(Node, node)->type == NODE_ARRAY) {
        YogVal unit = PTR_AS(Node, node)->u.array.type;
        RETURN(env, get_alignment_unit_of_node(env, unit));
    }
    RETURN(env, get_alignment_unit_of_node(env, node));
}

static uint_t
align_offset(YogEnv* env, YogVal node, uint_t offset)
{
    SAVE_ARG(env, node);
    uint_t size = get_alignment_unit(env, node);
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

static void
check_Fixnum_positive(YogEnv* env, YogVal n)
{
    if (0 <= VAL2INT(n)) {
        return;
    }
    const char* fmt = "Value must be greater or equal 0, not %d";
    YogError_raise_ValueError(env, fmt, VAL2INT(n));
}

static void
check_Bignum_is_greater_or_equal_than_int(YogEnv* env, YogVal bignum, int_t n)
{
    if (0 <= YogBignum_compare_with_int(env, bignum, n)) {
        return;
    }
    const char* fmt = "Value must be greater or equal %d, not %D";
    YogError_raise_ValueError(env, fmt, n, bignum);
}

static void
check_Bignum_is_less_or_equal_than_unsigned_int(YogEnv* env, YogVal bignum, uint_t n)
{
    if (YogBignum_compare_with_unsigned_int(env, bignum, n) <= 0) {
        return;
    }
    const char* fmt = "Value must be less or equal %u, not %D";
    YogError_raise_ValueError(env, fmt, n, bignum);
}

static void
check_Bignum_uint(YogEnv* env, YogVal val)
{
    check_Bignum_is_greater_or_equal_than_int(env, val, 0);
    check_Bignum_is_less_or_equal_than_unsigned_int(env, val, UNSIGNED_MAX);
}

static void
init_struct_with_ptr(YogEnv* env, YogVal obj, void* ptr)
{
    PTR_AS(Struct, obj)->data = ptr;
    PTR_AS(Struct, obj)->own = FALSE;
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

    if (IS_NIL(ptr)) {
        klass = YogVal_get_class(env, self);
        uint_t size = PTR_AS(StructClass, klass)->size;
        PTR_AS(Struct, self)->data = YogGC_malloc(env, size);
        bzero(PTR_AS(Struct, self)->data, size);
        RETURN(env, self);
    }
    if (IS_FIXNUM(ptr)) {
        check_Fixnum_positive(env, ptr);
        init_struct_with_ptr(env, self, (void*)VAL2INT(ptr));
        RETURN(env, self);
    }
    if (IS_PTR(ptr) && (BASIC_OBJ_TYPE(ptr) == TYPE_BIGNUM)) {
        check_Bignum_uint(env, ptr);
        void* data = (void*)YogBignum_to_unsigned_type(env, ptr, "ptr");
        init_struct_with_ptr(env, self, data);
        RETURN(env, self);
    }

    const char* fmt = "ptr must be Nil, Fixnum or Bignum, not %C";
    YogError_raise_TypeError(env, fmt, ptr);
    RETURN(env, YUNDEF);
}

typedef YogVal (*StructBuilder)(YogEnv*, ID, YogVal);
static YogVal build_StructClass(YogEnv*, ID, YogVal);
static YogVal build_UnionClass(YogEnv*, ID, YogVal);

static YogVal
create_anonymous_struct_node(YogEnv* env, YogVal type, StructBuilder builder)
{
    SAVE_ARG(env, type);
    YogVal node = Node_new(env);
    PUSH_LOCAL(env, node);

    PTR_AS(Node, node)->type = NODE_STRUCT;
    YogVal fields = get_fields_of_anonymous_struct(env, type);
    YogVal klass = builder(env, INVALID_ID, fields);
    YogGC_UPDATE_PTR(env, PTR_AS(Node, node), u.struct_.klass, klass);

    RETURN(env, node);
}

static YogVal
create_atom_node(YogEnv* env, ID type)
{
    YogVal node = Node_new(env);
    PTR_AS(Node, node)->type = NODE_ATOM;
    PTR_AS(Node, node)->u.atom.type = type;
    return node;
}

static ID
intern_pointer(YogEnv* env)
{
    return YogVM_intern(env, env->vm, "pointer");
}

static ID
intern_void(YogEnv* env)
{
    return YogVM_intern(env, env->vm, "void");
}

static YogVal
parse_pointer(YogEnv* env, YogVal subinfo)
{
    SAVE_ARG(env, subinfo);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    if (IS_SYMBOL(subinfo) && (VAL2ID(subinfo) == intern_void(env))) {
        RETURN(env, create_atom_node(env, intern_pointer(env)));
    }
    node = Node_new(env);
    PTR_AS(Node, node)->type = NODE_POINTER;
    YogGC_UPDATE_PTR(env, PTR_AS(Node, node), u.pointer.klass, subinfo);
    RETURN(env, node);
}

static YogVal
parse_subtype(YogEnv* env, YogVal type)
{
    SAVE_ARG(env, type);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    YogVal name = YogArray_at(env, type, 0);
    if (!IS_SYMBOL(name)) {
        RETURN(env, YUNDEF);
    }
    YogVM* vm = env->vm;
    if (VAL2ID(name) == YogVM_intern(env, vm, "string")) {
        node = Node_new(env);
        PTR_AS(Node, node)->type = NODE_STRING;
        YogVal encoding = YogArray_at(env, type, 1);
        const char* name = "Second element of type";
        YogMisc_check_Encoding(env, VAL2HDL(env, encoding), name);
        YogGC_UPDATE_PTR(env, PTR_AS(Node, node), u.string.encoding, encoding);
        RETURN(env, node);
    }
    ID sym = VAL2ID(name);
    if (sym == YogVM_intern(env, vm, "struct")) {
        RETURN(env, create_anonymous_struct_node(env, type, build_StructClass));
    }
    if (sym == YogVM_intern(env, vm, "union")) {
        RETURN(env, create_anonymous_struct_node(env, type, build_UnionClass));
    }
    if (sym == YogVM_intern(env, env->vm, "pointer")) {
        RETURN(env, parse_pointer(env, YogArray_at(env, type, 1)));
    }
    RETURN(env, YUNDEF);
}

static YogVal
parse_type(YogEnv* env, YogVal type)
{
    SAVE_ARG(env, type);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    if (IS_SYMBOL(type)) {
        RETURN(env, create_atom_node(env, VAL2ID(type)));
    }
#define RAISE_TYPE_ERROR do { \
    const char* fmt = "Field must be Symbol or Array, not %C"; \
    YogError_raise_TypeError(env, fmt, type); \
} while (0)
    if (!IS_PTR(type)) {
        RAISE_TYPE_ERROR;
    }
    YogVM* vm = env->vm;
    if (type == vm->cBuffer) {
        node = Node_new(env);
        PTR_AS(Node, node)->type = NODE_BUFFER;
        RETURN(env, node);
    }
    if (BASIC_OBJ_TYPE(type) == TYPE_STRUCT_CLASS) {
        node = Node_new(env);
        PTR_AS(Node, node)->type = NODE_STRUCT;
        YogGC_UPDATE_PTR(env, PTR_AS(Node, node), u.struct_.klass, type);
        RETURN(env, node);
    }
    if (BASIC_OBJ_TYPE(type) != TYPE_ARRAY) {
        RAISE_TYPE_ERROR;
    }
#undef RAISE_TYPE_ERROR
    uint_t n = YogArray_size(env, type);
    if (n != 2) {
        const char* fmt = "Array field must be length of two, not %u";
        YogError_raise_ValueError(env, fmt, n);
    }
    node = parse_subtype(env, type);
    if (!IS_UNDEF(node)) {
        RETURN(env, node);
    }
    YogVal size = YogArray_at(env, type, 1);
    if (!IS_FIXNUM(size)) {
        const char* fmt = "Array field size must be Fixnum, not %C";
        YogError_raise_TypeError(env, fmt, size);
    }
    if (VAL2INT(size) < 0) {
        const char* fmt = "Array field size must be positive, not %d";
        YogError_raise_ValueError(env, fmt, VAL2INT(size));
    }
    node = Node_new(env);
    PTR_AS(Node, node)->type = NODE_ARRAY;
    PTR_AS(Node, node)->u.array.size = VAL2INT(size);
    YogVal child = parse_type(env, YogArray_at(env, type, 0));
    YogGC_UPDATE_PTR(env, PTR_AS(Node, node), u.array.type, child);
    RETURN(env, node);
}

static YogVal
parse_field(YogEnv* env, YogVal field)
{
    SAVE_ARG(env, field);
    YogVal node = YUNDEF;
    YogVal type = YUNDEF;
    PUSH_LOCALS2(env, node, type);
    if (!IS_PTR(field) || (BASIC_OBJ_TYPE(field) != TYPE_ARRAY)) {
        YogError_raise_TypeError(env, "Field must be Array, not %C", field);
    }
    uint_t size = YogArray_size(env, field);
    if (size < 2) {
        YogError_raise_ValueError(env, "Field size must be two, not %u", size);
    }
    node = Node_new(env);
    PTR_AS(Node, node)->type = NODE_FIELD;
    PTR_AS(Node, node)->u.field.next = YUNDEF;
    type = parse_type(env, YogArray_at(env, field, 0));
    YogGC_UPDATE_PTR(env, PTR_AS(Node, node), u.field.type, type);
    YogVal name = YogArray_at(env, field, 1);
    if (!IS_SYMBOL(name)) {
        const char* fmt = "Field name must be Symbol, not %C";
        YogError_raise_TypeError(env, fmt, name);
    }
    PTR_AS(Node, node)->u.field.name = VAL2ID(name);
    RETURN(env, node);
}

static YogVal
parse_fields(YogEnv* env, YogVal fields)
{
    SAVE_ARG(env, fields);
    YogVal anchor = Node_new(env);
    YogVal node = YUNDEF;
    YogVal next = YUNDEF;
    PUSH_LOCALS3(env, anchor, node, next);

    node = anchor;
    uint_t size = YogArray_size(env, fields);
    uint_t i;
    for (i = 0; i < size; i++) {
        next = parse_field(env, YogArray_at(env, fields, i));
        YogGC_UPDATE_PTR(env, PTR_AS(Node, node), u.field.next, next);
        node = next;
    }

    RETURN(env, PTR_AS(Node, anchor)->u.field.next);
}

static uint_t
get_total_field_size(YogEnv* env, YogVal node)
{
    YogVal type = PTR_AS(Node, node)->u.field.type;
    if (PTR_AS(Node, type)->type == NODE_ARRAY) {
        uint_t size = PTR_AS(Node, type)->u.array.size;
        return get_size(env, PTR_AS(Node, type)->u.array.type) * size;
    }
    return get_size(env, type);
}

static uint_t
get_children_num(YogEnv* env, YogVal node)
{
    YogVal type = PTR_AS(Node, node)->u.field.type;
    NodeType nt = PTR_AS(Node, type)->type;
    return (nt == NODE_POINTER) || (nt == NODE_BUFFER) ? 1 : 0;
}

static void
update_max(uint_t* max, uint_t n)
{
    if (n <= *max) {
        return;
    }
    *max = n;
}

static uint_t
get_union_size(YogEnv* env, YogVal nodes)
{
    SAVE_ARG(env, nodes);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    uint_t max = 0;
    for (node = nodes; IS_PTR(node); node = PTR_AS(Node, node)->u.field.next) {
        update_max(&max, get_total_field_size(env, node));
    }

    RETURN(env, max);
}

static uint_t
get_alignment_unit_of_union(YogEnv* env, YogVal nodes)
{
    SAVE_ARG(env, nodes);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    uint_t max = 0;
    for (node = nodes; IS_PTR(node); node = PTR_AS(Node, node)->u.field.next) {
        update_max(&max, get_alignment_unit(env, node));
    }

    RETURN(env, max);
}

static YogVal
create_UnionClass(YogEnv* env, ID name)
{
    YogVal obj = ALLOC_OBJ(env, YogClass_keep_children, NULL, StructClass);
    UnionClass_init(env, obj);
    PTR_AS(YogClass, obj)->name = name;
    return obj;
}

static void
define_fields_to_UnionClass(YogEnv* env, YogVal self, YogVal fields)
{
    SAVE_ARGS2(env, self, fields);
    YogVal nodes = parse_fields(env, fields);
    YogVal node = YUNDEF;
    PUSH_LOCALS2(env, nodes, node);

    uint_t alignment_unit = get_alignment_unit_of_union(env, nodes);
    PTR_AS(StructClass, self)->alignment_unit = alignment_unit;

    uint_t children_num = 0;
    for (node = nodes; IS_PTR(node); node = PTR_AS(Node, node)->u.field.next) {
        StructClass_set_field(env, self, node, 0, children_num);
        children_num += get_children_num(env, node);
    }
    uint_t union_size = get_union_size(env, nodes);
    PTR_AS(StructClass, self)->size = union_size;
    PTR_AS(StructClass, self)->children_num = children_num;
    RETURN_VOID(env);
}

static YogVal
build_UnionClass(YogEnv* env, ID name, YogVal fields)
{
    SAVE_ARG(env, fields);
    YogVal obj = create_UnionClass(env, name);
    PUSH_LOCAL(env, obj);

    define_fields_to_UnionClass(env, obj, fields);

    RETURN(env, obj);
}

static YogVal
create_StructClass(YogEnv* env, ID name)
{
    YogVal obj = ALLOC_OBJ(env, YogClass_keep_children, NULL, StructClass);
    StructClass_init(env, obj);
    PTR_AS(YogClass, obj)->name = name;
    return obj;
}

static void
define_fields_to_StructClass(YogEnv* env, YogVal self, YogVal fields)
{
    SAVE_ARGS2(env, self, fields);
    YogVal nodes = parse_fields(env, fields);
    YogVal node = YUNDEF;
    PUSH_LOCALS2(env, nodes, node);

    uint_t alignment_unit = get_alignment_unit(env, nodes);
    PTR_AS(StructClass, self)->alignment_unit = alignment_unit;

    uint_t offset = 0;
    uint_t children_num = 0;
    for (node = nodes; IS_PTR(node); node = PTR_AS(Node, node)->u.field.next) {
        offset = align_offset(env, node, offset);
        StructClass_set_field(env, self, node, offset, children_num);
        offset += get_total_field_size(env, node);
        children_num += get_children_num(env, node);
    }
    PTR_AS(StructClass, self)->size = offset;
    PTR_AS(StructClass, self)->children_num = children_num;

    RETURN_VOID(env);
}

static YogVal
build_StructClass(YogEnv* env, ID name, YogVal fields)
{
    SAVE_ARG(env, fields);
    YogVal obj = create_StructClass(env, name);
    PUSH_LOCAL(env, obj);

    define_fields_to_StructClass(env, obj, fields);

    RETURN(env, obj);
}

static YogVal
UnionClass_new(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal fields = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS2(env, name, fields);
    YogCArg params[] = {
        { "name", &name },
        { "|", NULL },
        { "fields", &fields },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "new", params, args, kw);
    if (!IS_PTR(name) || (BASIC_OBJ_TYPE(name) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "name must be String, not %C", name);
    }
    ID id = YogVM_intern2(env, env->vm, name);
    if (IS_UNDEF(fields)) {
        RETURN(env, create_UnionClass(env, name));
    }
    if (!IS_PTR(fields) || (BASIC_OBJ_TYPE(fields) != TYPE_ARRAY)) {
        YogError_raise_TypeError(env, "fields must be Array, not %C", fields);
    }

    RETURN(env, build_UnionClass(env, id, fields));
}

static YogVal
StructClass_define_fields(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal fields = YUNDEF;
    PUSH_LOCAL(env, fields);
    YogCArg params[] = { { "fields", &fields }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "define_fields", params, args, kw);
    CHECK_SELF_STRUCT_CLASS;

    define_fields_to_StructClass(env, self, fields);

    RETURN(env, self);
}

static YogVal
StructClass_new(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal fields = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS2(env, name, fields);
    YogCArg params[] = {
        { "name", &name },
        { "|", NULL },
        { "fields", &fields },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "new", params, args, kw);
    if (!IS_PTR(name) || (BASIC_OBJ_TYPE(name) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "name must be String, not %C", name);
    }
    ID id = YogVM_intern2(env, env->vm, name);
    if (IS_UNDEF(fields)) {
        RETURN(env, create_StructClass(env, name));
    }
    if (!IS_PTR(fields) || (BASIC_OBJ_TYPE(fields) != TYPE_ARRAY)) {
        YogError_raise_TypeError(env, "fields must be Array, not %C", fields);
    }

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
    uint_t children_num = PTR_AS(StructClass, klass)->children_num;
    uint_t i;
    for (i = 0; i < children_num; i++) {
        YogGC_KEEP(env, st, children[i], keeper, heap);
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

    uint_t children_num = PTR_AS(StructClass, klass)->children_num;
    YogGC_check_multiply_overflow(env, children_num, sizeof(YogVal));
    obj = ALLOC_OBJ_ITEM(env, Struct_keep_children, Struct_finalize, Struct, children_num, YogVal);
    YogBasicObj_init(env, obj, TYPE_STRUCT, 0, klass);
    PTR_AS(Struct, obj)->data = NULL;
    PTR_AS(Struct, obj)->own = TRUE;
    PTR_AS(Struct, obj)->top = obj;
    uint_t i;
    for (i = 0; i < children_num; i++) {
        PTR_AS(Struct, obj)->children[i] = YNIL;
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
    YogGC_KEEP(env, f, rtype, keeper, heap);
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
    PTR_AS(LibFunc, obj)->rtype = YUNDEF;
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
map_type(YogEnv* env, YogVal node)
{
    switch (PTR_AS(Node, node)->type) {
    case NODE_ARRAY:
        YogError_raise_FFIError(env, "Array argument is not supported");
    case NODE_ATOM:
        return map_id_type(env, PTR_AS(Node, node)->u.atom.type);
    case NODE_BUFFER:
    case NODE_POINTER:
    case NODE_STRING:
        return &ffi_type_pointer;
    case NODE_STRUCT:
        YogError_raise_FFIError(env, "Struct argument is not supported");
    case NODE_FIELD:
    default:
        YOG_BUG(env, "Invalid Node type (%u)", PTR_AS(Node, node)->type);
    }

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

static void
check_rtype_class(YogEnv* env, YogVal rtype)
{
    if (IS_NIL(rtype) || IS_SYMBOL(rtype)) {
        return;
    }
    if (IS_PTR(rtype) && (BASIC_OBJ_TYPE(rtype) == TYPE_ARRAY)) {
        return;
    }
    YogError_raise_TypeError(env, "rtype must be Symbol or nil, not %C", rtype);
}

static BOOL
is_void(YogEnv* env, YogVal sym)
{
    return IS_SYMBOL(sym) && (VAL2ID(sym) == intern_void(env));
}

static YogVal
rtype2node(YogEnv* env, YogVal rtype)
{
    SAVE_ARG(env, rtype);
    if (IS_NIL(rtype) || is_void(env, rtype)) {
        RETURN(env, YNIL);
    }
    RETURN(env, parse_type(env, rtype));
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
    YogVal node = YUNDEF;
    PUSH_LOCALS6(env, f, name, arg_types, rtype, arg_type, node);
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
    check_rtype_class(env, rtype);

    uint_t nargs = IS_NIL(arg_types) ? 0 : YogArray_size(env, arg_types);
    f = LibFunc_new(env, nargs);
    ffi_type** types = (ffi_type**)YogGC_malloc(env, sizeof(ffi_type*) * nargs);
    uint_t i;
    for (i = 0; i < nargs; i++) {
        arg_type = YogArray_at(env, arg_types, i);
        node = parse_type(env, arg_type);
        types[i] = map_type(env, node);
        YogGC_UPDATE_PTR(env, PTR_AS(LibFunc, f), arg_types[i], node);
    }
    node = rtype2node(env, rtype);
    YogGC_UPDATE_PTR(env, PTR_AS(LibFunc, f), rtype, node);
    ffi_status status = ffi_prep_cif(&PTR_AS(LibFunc, f)->cif, FFI_DEFAULT_ABI, nargs, IS_NIL(node) ? &ffi_type_void : map_type(env, node), types);
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
    if (!IS_FIXNUM(val)) {
        YogError_raise_TypeError(env, "Value must be Fixnum, not %C", val);
    }
    if (VAL2INT(val) < min) {
        const char* fmt = "Value must be greater or equal %d, not %D";
        YogError_raise_ValueError(env, fmt, min, val);
    }
    if (max < VAL2INT(val)) {
        const char* fmt = "Value must be less or equal %d, not %D";
        YogError_raise_ValueError(env, fmt, max, val);
    }
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
check_Bignum_is_greater_or_equal_than_long(YogEnv* env, YogVal bignum, long n)
{
    if (0 <= YogBignum_compare_with_long(env, bignum, n)) {
        return;
    }
    const char* fmt = "Value must be greater or equal %d, not %D";
    YogError_raise_ValueError(env, fmt, n, bignum);
}

static void
check_Bignum_is_less_or_equal_than_unsigned_long(YogEnv* env, YogVal bignum, unsigned long n)
{
    if (YogBignum_compare_with_unsigned_long(env, bignum, n) <= 0) {
        return;
    }
    const char* fmt = "Value must be less or equal %lu, not %D";
    YogError_raise_ValueError(env, fmt, n, bignum);
}

static void
check_Bignum_is_less_or_equal_than_unsigned_long_long(YogEnv* env, YogVal bignum, unsigned long long n)
{
    if (YogBignum_compare_with_unsigned_long_long(env, bignum, n) <= 0) {
        return;
    }
    const char* fmt = "Value must be less or equal %llu, not %D";
    YogError_raise_ValueError(env, fmt, n, bignum);
}

static void
check_Bignum_unsigned_long_long(YogEnv* env, YogVal val)
{
    check_Bignum_is_greater_or_equal_than_int(env, val, 0);
    check_Bignum_is_less_or_equal_than_unsigned_long_long(env, val, ULLONG_MAX);
}

static void
check_Bignum_unsigned_long(YogEnv* env, YogVal val)
{
    check_Bignum_is_greater_or_equal_than_int(env, val, 0);
    check_Bignum_is_less_or_equal_than_unsigned_long(env, val, ULONG_MAX);
}

static void
check_Bignum_unsigned_int(YogEnv* env, YogVal val)
{
    check_Bignum_is_greater_or_equal_than_int(env, val, 0);
    check_Bignum_is_less_or_equal_than_unsigned_int(env, val, UINT_MAX);
}

static void
check_Bignum_uint32(YogEnv* env, YogVal val)
{
    SAVE_ARG(env, val);
    check_Bignum_is_greater_or_equal_than_int(env, val, 0);
    check_Bignum_is_less_or_equal_than_unsigned_int(env, val, UINT32_MAX);
    RETURN_VOID(env);
}

static void
check_Bignum_is_less_or_equal_than_long(YogEnv* env, YogVal val, long n)
{
    if (YogBignum_compare_with_long(env, val, n) <= 0) {
        return;
    }
    const char* fmt = "Value must be less or equal %d, not %D";
    YogError_raise_ValueError(env, fmt, n, val);
}

static void
check_Bignum_is_less_or_equal_than_int(YogEnv* env, YogVal val, int_t n)
{
    if (YogBignum_compare_with_int(env, val, n) <= 0) {
        return;
    }
    const char* fmt = "Value must be less or equal %d, not %D";
    YogError_raise_ValueError(env, fmt, n, val);
}

static void
check_Bignum_is_greater_or_equal_than_long_long(YogEnv* env, YogVal bignum, long long n)
{
    if (0 <= YogBignum_compare_with_long_long(env, bignum, n)) {
        return;
    }
    const char* fmt = "Value must be greater or equal %lld, not %D";
    YogError_raise_ValueError(env, fmt, n, bignum);
}

static void
check_Bignum_is_less_or_equal_than_long_long(YogEnv* env, YogVal bignum, long long n)
{
    if (YogBignum_compare_with_long_long(env, bignum, INT64_MAX) <= 0) {
        return;
    }
    const char* fmt = "Value must be less or equal %llu, not %D";
    YogError_raise_ValueError(env, fmt, n, bignum);
}

static void
check_Bignum_long_long(YogEnv* env, YogVal val)
{
    check_Bignum_is_greater_or_equal_than_long_long(env, val, LLONG_MIN);
    check_Bignum_is_less_or_equal_than_long_long(env, val, LLONG_MAX);
}

static void
check_Bignum_long(YogEnv* env, YogVal val)
{
    check_Bignum_is_greater_or_equal_than_long(env, val, LONG_MIN);
    check_Bignum_is_less_or_equal_than_long(env, val, LONG_MAX);
}

static void
check_Bignum_int(YogEnv* env, YogVal val)
{
    check_Bignum_is_greater_or_equal_than_int(env, val, INT_MIN);
    check_Bignum_is_less_or_equal_than_int(env, val, INT_MAX);
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
raise_TypeError_for_int(YogEnv* env, YogVal val)
{
    const char* fmt = "Value must be Fixnum or Bignum, not %C";
    YogError_raise_TypeError(env, fmt, val);
}

static void
write_uint32(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    uint32_t* p = (uint32_t*)dest;
    if (IS_FIXNUM(val)) {
        check_Fixnum_positive(env, val);
        *p = VAL2INT(val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_uint32(env, val);
        *p = YogBignum_to_unsigned_type(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

    RETURN_VOID(env);
}

static void
write_int32(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    int32_t* p = (int32_t*)dest;
    if (IS_FIXNUM(val)) {
        *p = VAL2INT(val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_int32(env, val);
        *p = YogBignum_to_signed_type(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

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
write_uint64_Fixnum(YogEnv* env, void* dest, YogVal val)
{
    if (VAL2INT(val) < 0) {
        const char* fmt = "Value must be greater or equal 0, not %d";
        YogError_raise_ValueError(env, fmt, VAL2INT(val));
    }
    uint64_t* p = dest;
    *p = VAL2INT(val);
}

static void
write_uint64(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    if (IS_FIXNUM(val)) {
        write_uint64_Fixnum(env, dest, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_uint64(env, val);
        uint64_t* p = (uint64_t*)dest;
        *p = YogBignum_to_unsigned_long_long(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

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
write_int64(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    int64_t* p = (int64_t*)dest;
    if (IS_FIXNUM(val)) {
        *p = VAL2INT(val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_int64(env, val);
        *p = YogBignum_to_long_long(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

    RETURN_VOID(env);
}

static void
write_double(YogEnv* env, void* dest, YogVal val)
{
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_FLOAT)) {
        double* p = (double*)dest;
        *p = FLOAT_NUM(val);
        return;
    }
    YogError_raise_TypeError(env, "Value must be Float, not %C", val);
}

static void
write_long_double(YogEnv* env, void* dest, YogVal val)
{
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_FLOAT)) {
        long double* p = (long double*)dest;
        *p = FLOAT_NUM(val);
        return;
    }
    YogError_raise_TypeError(env, "Value must be Float, not %C", val);
}

static void
write_float(YogEnv* env, void* dest, YogVal val)
{
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_FLOAT)) {
        float* p = (float*)dest;
        *p = FLOAT_NUM(val);
        return;
    }
    YogError_raise_TypeError(env, "Value must be Float, not %C", val);
}

static void
write_int8(YogEnv* env, void* dest, YogVal val)
{
    check_Fixnum_int8(env, val);
    *((int8_t*)dest) = VAL2INT(val);
}

static void
write_char(YogEnv* env, void* dest, YogVal val)
{
    check_Fixnum(env, val, CHAR_MIN, CHAR_MAX);
    char* p = (char*)dest;
    *p = VAL2INT(val);
}

static void
write_uchar(YogEnv* env, void* dest, YogVal val)
{
    check_Fixnum(env, val, 0, UCHAR_MAX);
    unsigned char* p = (unsigned char*)dest;
    *p = VAL2INT(val);
}

static void
write_uint8(YogEnv* env, void* dest, YogVal val)
{
    check_Fixnum_uint8(env, val);
    *((uint8_t*)dest) = VAL2INT(val);
}

static void
write_uint16(YogEnv* env, void* dest, YogVal val)
{
    check_Fixnum_uint16(env, val);
    *((uint16_t*)dest) = VAL2INT(val);
}

static void
write_int16(YogEnv* env, void* dest, YogVal val)
{
    check_Fixnum_int16(env, val);
    *((int16_t*)dest) = VAL2INT(val);
}

static void
write_argument_Buffer(YogEnv* env, void** ptr, YogVal val)
{
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_BUFFER)) {
        YogError_raise_TypeError(env, "Argument must be Buffer, not %C", val);
    }
    *ptr = PTR_AS(Buffer, val)->ptr;
}

static void
write_argument_Struct(YogEnv* env, void* ptr, YogVal arg_type, YogVal val)
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
    *((void**)ptr) = PTR_AS(Struct, val)->data;
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
write_ushort(YogEnv* env, void* dest, YogVal val)
{
    check_Fixnum(env, val, 0, USHRT_MAX);
    unsigned short* p = (unsigned short*)dest;
    *p = VAL2INT(val);
}

static void
write_short(YogEnv* env, void* dest, YogVal val)
{
    check_Fixnum(env, val, SHRT_MIN, SHRT_MAX);
    short* p = (short*)dest;
    *p = VAL2INT(val);
}

#define WRITE_POSITIVE_NUM(env, type, dest, val) do { \
    check_Fixnum_positive((env), (val)); \
    type* p = (type*)dest; \
    *p = (type)VAL2INT((val)); \
} while (0)

static void
write_ulonglong(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    typedef unsigned long long TargetType;
    if (IS_FIXNUM(val)) {
        WRITE_POSITIVE_NUM(env, TargetType, dest, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_unsigned_long_long(env, val);
        TargetType* p = (TargetType*)dest;
        *p = YogBignum_to_unsigned_long_long(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

    RETURN_VOID(env);
}

static void
write_ulong(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    typedef unsigned long TargetType;
    if (IS_FIXNUM(val)) {
        WRITE_POSITIVE_NUM(env, TargetType, dest, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_unsigned_long(env, val);
        TargetType* p = (TargetType*)dest;
        *p = YogBignum_to_unsigned_long(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

    RETURN_VOID(env);
}

static void
write_longlong(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    typedef long long TargetType;
    TargetType* p = (TargetType*)dest;
    if (IS_FIXNUM(val)) {
        *p = VAL2INT(val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_long_long(env, val);
        *p = YogBignum_to_long_long(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

    RETURN_VOID(env);
}

static void
write_long(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    long* p = (long*)dest;
    if (IS_FIXNUM(val)) {
        *p = VAL2INT(val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_long(env, val);
        *p = YogBignum_to_long(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

    RETURN_VOID(env);
}

static void
write_pointer(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    typedef void* TargetType;
    if (IS_FIXNUM(val)) {
        WRITE_POSITIVE_NUM(env, TargetType, dest, val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_uint(env, val);
        TargetType* p = (TargetType*)dest;
        *p = (TargetType)YogBignum_to_unsigned_type(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

    RETURN_VOID(env);
}

static void
write_uint(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    unsigned int* p = (unsigned int*)dest;
    if (IS_FIXNUM(val)) {
        check_Fixnum_positive(env, val);
        *p = VAL2INT(val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_unsigned_int(env, val);
        *p = YogBignum_to_unsigned_type(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

    RETURN_VOID(env);
}

static void
write_int(YogEnv* env, void* dest, YogVal val)
{
    SAVE_ARG(env, val);

    int* p = (int*)dest;
    if (IS_FIXNUM(val)) {
        *p = VAL2INT(val);
        RETURN_VOID(env);
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        check_Bignum_int(env, val);
        *p = YogBignum_to_signed_type(env, val, "Value");
        RETURN_VOID(env);
    }
    raise_TypeError_for_int(env, val);

    RETURN_VOID(env);
}

static BOOL
write_data(YogEnv* env, void* dest, ID type, YogVal val)
{
    SAVE_ARG(env, val);
    const char* s = BINARY_CSTR(YogVM_id2bin(env, env->vm, type));
    if (strcmp(s, "uint8") == 0) {
        write_uint8(env, dest, val);
    }
    else if (strcmp(s, "int8") == 0) {
        write_int8(env, dest, val);
    }
    else if (strcmp(s, "uint16") == 0) {
        write_uint16(env, dest, val);
    }
    else if (strcmp(s, "int16") == 0) {
        write_int16(env, dest, val);
    }
    else if (strcmp(s, "uint32") == 0) {
        write_uint32(env, dest, val);
    }
    else if (strcmp(s, "int32") == 0) {
        write_int32(env, dest, val);
    }
    else if (strcmp(s, "uint64") == 0) {
        write_uint64(env, dest, val);
    }
    else if (strcmp(s, "int64") == 0) {
        write_int64(env, dest, val);
    }
    else if (strcmp(s, "float") == 0) {
        write_float(env, dest, val);
    }
    else if (strcmp(s, "double") == 0) {
        write_double(env, dest, val);
    }
    else if (strcmp(s, "uchar") == 0) {
        write_uchar(env, dest, val);
    }
    else if (strcmp(s, "char") == 0) {
        write_char(env, dest, val);
    }
    else if (strcmp(s, "ushort") == 0) {
        write_ushort(env, dest, val);
    }
    else if (strcmp(s, "short") == 0) {
        write_short(env, dest, val);
    }
    else if (strcmp(s, "uint") == 0) {
        write_uint(env, dest, val);
    }
    else if (strcmp(s, "int") == 0) {
        write_int(env, dest, val);
    }
    else if (strcmp(s, "ulong") == 0) {
        write_ulong(env, dest, val);
    }
    else if (strcmp(s, "long") == 0) {
        write_long(env, dest, val);
    }
    else if (strcmp(s, "ulonglong") == 0) {
        write_ulonglong(env, dest, val);
    }
    else if (strcmp(s, "longlong") == 0) {
        write_longlong(env, dest, val);
    }
    else if (strcmp(s, "longdouble") == 0) {
        write_long_double(env, dest, val);
    }
    else if (strcmp(s, "pointer") == 0) {
        write_pointer(env, dest, val);
    }
    else {
        RETURN(env, FALSE);
    }
    RETURN(env, TRUE);
}

static void
write_argument_pointer(YogEnv* env, void* pvalue, YogVal klass, YogVal val)
{
    if (IS_PTR(klass) && (BASIC_OBJ_TYPE(klass) == TYPE_STRUCT_CLASS)) {
        write_argument_Struct(env, pvalue, klass, val);
        return;
    }
    if (!IS_SYMBOL(klass)) {
        const char* fmt = "Pointer field must be Symbol or StructClass, not %C";
        YogError_raise_TypeError(env, fmt, klass);
    }
    ID field = VAL2ID(klass);
    if (field == intern_void(env)) {
        write_pointer(env, pvalue, val);
        return;
    }
    YogError_raise_ValueError(env, "Invalid pointer field: %I", klass);
}

static void
write_argument(YogEnv* env, void* pvalue, void* refered, YogVal node, YogVal val)
{
    SAVE_ARGS2(env, node, val);
    YogVal klass;
    YogHandle* encoding;
    YogVal bin;
    YogHandle* h;
    switch (PTR_AS(Node, node)->type) {
    case NODE_ATOM:
        break;
    case NODE_BUFFER:
        write_argument_Buffer(env, pvalue, val);
        RETURN_VOID(env);
    case NODE_POINTER:
        klass = PTR_AS(Node, node)->u.pointer.klass;
        write_argument_pointer(env, pvalue, klass, val);
        RETURN_VOID(env);
    case NODE_STRING:
        encoding = VAL2HDL(env, PTR_AS(Node, node)->u.string.encoding);
        h = VAL2HDL(env, val);
        YogMisc_check_String(env, h, "Actual parameter");
        bin = YogEncoding_conv_from_yog(env, encoding, h);
        memcpy(refered, BINARY_CSTR(bin), BINARY_SIZE(bin));
        *((char**)pvalue) = (char*)refered;
        RETURN_VOID(env);
    case NODE_ARRAY:
    case NODE_FIELD:
    case NODE_STRUCT:
    default:
        YOG_BUG(env, "Invalid Node type (%u)", PTR_AS(Node, node)->type);
    }

    ID name = PTR_AS(Node, node)->u.atom.type;
    if (write_data(env, pvalue, name, val)) {
        RETURN_VOID(env);
    }
    const char* s = BINARY_CSTR(YogVM_id2bin(env, env->vm, name));
    if (strcmp(s, "int_p") == 0) {
        Int_write(env, val, (int*)refered);
    }
    else if (strcmp(s, "pointer_p") == 0) {
        Pointer_write(env, val, (void**)refered);
    }
    else {
        YogError_raise_ValueError(env, "Unknown argument type - %I", name);
    }
    *((void**)pvalue) = refered;
    RETURN_VOID(env);
}

static uint_t
type2refered_size_of_string(YogEnv* env, YogVal node, YogVal arg)
{
    YogVal enc = PTR_AS(Node, node)->u.string.encoding;
    YogMisc_check_String(env, VAL2HDL(env, arg), "Argument");
    return PTR_AS(YogEncoding, enc)->max_size * STRING_SIZE(arg);
}

static uint_t
type2refered_size(YogEnv* env, YogVal node, YogVal arg)
{
    switch (PTR_AS(Node, node)->type) {
    case NODE_ATOM:
        break;
    case NODE_BUFFER:
    case NODE_POINTER:
        return 0;
    case NODE_STRING:
        return type2refered_size_of_string(env, node, arg);
    case NODE_ARRAY:
    case NODE_FIELD:
    case NODE_STRUCT:
    default:
        YOG_BUG(env, "Invalid Node type (%u)", PTR_AS(Node, node)->type);
    }

    ID name = PTR_AS(Node, node)->u.atom.type;
    const char* s = BINARY_CSTR(YogVM_id2bin(env, env->vm, name));
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
read_argument(YogEnv* env, YogVal obj, YogVal node, void* p)
{
    SAVE_ARGS2(env, obj, node);
    if (p == NULL) {
        RETURN_VOID(env);
    }
    switch (PTR_AS(Node, node)->type) {
    case NODE_ATOM:
        break;
    case NODE_BUFFER:
    case NODE_POINTER:
    case NODE_STRING:
        RETURN_VOID(env);
    case NODE_ARRAY:
    case NODE_FIELD:
    case NODE_STRUCT:
    default:
        YOG_BUG(env, "Invalid Node type (%u)", PTR_AS(Node, node)->type);
    }

    ID name = PTR_AS(Node, node)->u.atom.type;
    YogVal s = YogVM_id2bin(env, env->vm, name);
    if (strcmp(BINARY_CSTR(s), "int_p") == 0) {
        read_argument_int(env, obj, *((int*)p));
        RETURN_VOID(env);
    }
    if (strcmp(BINARY_CSTR(s), "pointer_p") == 0) {
        read_argument_pointer(env, obj, *((void**)p));
        RETURN_VOID(env);
    }
    YogError_raise_ValueError(env, "Unknown type - %I", name);
    RETURN_VOID(env);
}

static const char*
NodeType_to_s(YogEnv* env, NodeType type)
{
    switch (type) {
    case NODE_ARRAY:
        return "NODE_ARRAY";
    case NODE_ATOM:
        return "NODE_ATOM";
    case NODE_BUFFER:
        return "NODE_BUFFER";
    case NODE_FIELD:
        return "NODE_FIELD";
    case NODE_POINTER:
        return "NODE_POINTER";
    case NODE_STRING:
        return "NODE_STRING";
    case NODE_STRUCT:
        return "NODE_STRUCT";
    default:
        YogError_raise_FFIError(env, "Invalid NodeType: %u", type);
        return NULL;
    }
}

static YogVal
ptr2int_retval(YogEnv* env, YogVal node, void* rvalue)
{
    SAVE_ARG(env, node);
    ID type = PTR_AS(Node, node)->u.atom.type;
    if (type != intern_pointer(env)) {
        const char* fmt = "Return value must be pointer, not %I";
        YogError_raise_FFIError(env, fmt, type);
    }
    RETURN(env, YogVal_from_unsigned_int(env, (uint_t)rvalue));
}

static YogVal
create_ptr_retval(YogEnv* env, YogHandle* callee, void* rvalue)
{
    YogVal node = HDL_AS(LibFunc, callee)->rtype;
    NodeType type = PTR_AS(Node, node)->type;
    if (type == NODE_ATOM) {
        return ptr2int_retval(env, node, rvalue);
    }
    if (type != NODE_POINTER) {
        const char* fmt = "Node::type must be NODE_ATOM or NODE_POINTER, not %s";
        YogError_raise_FFIError(env, fmt, NodeType_to_s(env, type));
    }
    YogVal klass = PTR_AS(Node, node)->u.pointer.klass;
    if (IS_PTR(klass) && (BASIC_OBJ_TYPE(klass) == TYPE_STRUCT_CLASS)) {
        YogVal obj = StructBase_alloc(env, klass);
        init_struct_with_ptr(env, obj, rvalue);
        return obj;
    }
    const char* fmt = "rtype must be a pointer to Struct or void, not %C";
    YogError_raise_TypeError(env, fmt, klass);
    return YUNDEF;
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
        YogHandle* h_arg_type = VAL2HDL(env, arg_type);
        uint_t refered_size = type2refered_size(env, arg_type, posargs[i]->val);
        void* refered = 0 < refered_size ? YogSysdeps_alloca(refered_size): NULL;
        YogVal val = posargs[i]->val;
        write_argument(env, pvalue, refered, HDL2VAL(h_arg_type), val);
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
        return create_ptr_retval(env, callee, *((void**)rvalue));
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
Struct_get_child(YogEnv* env, YogVal self, uint_t index)
{
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "Object must be Struct, not %C", self);
    }
    return PTR_AS(Struct, self)->children[index];
}

static YogVal
Struct_get_Buffer(YogEnv* env, YogVal self, YogVal field)
{
    if (!IS_PTR(field) || (BASIC_OBJ_TYPE(field) != TYPE_BUFFER_FIELD)) {
        const char* fmt = "Attribute must be BufferField, not %C";
        YogError_raise_TypeError(env, fmt, field);
    }

    uint_t index = PTR_AS(BufferField, field)->child_index;
    return Struct_get_child(env, self, index);
}

static YogVal
Struct_get_pointer(YogEnv* env, YogVal self, YogVal field)
{
    if (!IS_PTR(field) || (BASIC_OBJ_TYPE(field) != TYPE_POINTER_FIELD)) {
        const char* fmt = "Attribute must be PointerField, not %C";
        YogError_raise_TypeError(env, fmt, field);
    }

    uint_t index = PTR_AS(PointerField, field)->child_index;
    return Struct_get_child(env, self, index);
}

static YogVal
Struct_get_String(YogEnv* env, YogVal self, YogVal field)
{
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "self must be Struct, not %C", self);
    }
    if (!IS_PTR(field) || (BASIC_OBJ_TYPE(field) != TYPE_STRING_FIELD)) {
        const char* fmt = "Attribute must be StringField, not %C";
        YogError_raise_TypeError(env, fmt, field);
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
        val = INT2VAL(*((unsigned char*)ptr));
    }
    else if (strcmp(s, "char") == 0) {
        val = INT2VAL(*((char*)ptr));
    }
    else if (strcmp(s, "ushort") == 0) {
        val = INT2VAL(*((unsigned short*)ptr));
    }
    else if (strcmp(s, "short") == 0) {
        val = INT2VAL(*((short*)ptr));
    }
    else if (strcmp(s, "uint") == 0) {
        val = YogVal_from_unsigned_int(env, *((unsigned int*)ptr));
    }
    else if (strcmp(s, "int") == 0) {
        val = YogVal_from_int(env, *((int*)ptr));
    }
    else if (strcmp(s, "ulong") == 0) {
        val = YogVal_from_unsigned_int(env, *((unsigned long*)ptr));
    }
    else if (strcmp(s, "long") == 0) {
        val = YogVal_from_int(env, *((long*)ptr));
    }
    else if (strcmp(s, "ulonglong") == 0) {
        val = YogVal_from_unsigned_long_long(env, *((unsigned long long*)ptr));
    }
    else if (strcmp(s, "longlong") == 0) {
        val = YogVal_from_long_long(env, *((long long*)ptr));
    }
    else if (strcmp(s, "longdouble") == 0) {
        val = YogFloat_from_float(env, *((long double*)ptr));
    }
    else if (strcmp(s, "pointer") == 0) {
        val = YogVal_from_unsigned_int(env, *((uint_t*)ptr));
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
    return Struct_get_Buffer(env, obj, attr);
}

static void
BufferField_exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    YogVal val = Struct_get_Buffer(env, obj, attr);
    YogScriptFrame_push_stack(env, env->frame, val);
}

static YogVal
PointerField_call_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    return Struct_get_pointer(env, obj, attr);
}

static void
PointerField_exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    YogVal val = Struct_get_pointer(env, obj, attr);
    YogScriptFrame_push_stack(env, env->frame, val);
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

static void
Struct_write_child(YogEnv* env, YogVal self, uint_t index, YogVal obj, uint_t offset, void* ptr)
{
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRUCT)) {
        YogError_raise_TypeError(env, "Object must be Struct, not %C", self);
    }
    YogGC_UPDATE_PTR(env, PTR_AS(Struct, self), children[index], obj);
    void** dest = PTR_AS(Struct, self)->data + offset;
    *dest = ptr;
}

static void
Struct_write_Buffer(YogEnv* env, YogVal self, YogVal field, YogVal child)
{
    uint_t index = PTR_AS(BufferField, field)->child_index;
    uint_t offset = PTR_AS(FieldBase, field)->offset;
    void* ptr = PTR_AS(Buffer, child)->ptr;
    Struct_write_child(env, self, index, child, offset, ptr);
}

static void
Struct_write_pointer(YogEnv* env, YogVal self, YogVal field, YogVal child)
{
    uint_t index = PTR_AS(PointerField, field)->child_index;
    uint_t offset = PTR_AS(FieldBase, field)->offset;
    void* ptr = PTR_AS(Struct, child)->data;
    Struct_write_child(env, self, index, child, offset, ptr);
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
    if (!IS_PTR(attr) || (BASIC_OBJ_TYPE(attr) != TYPE_BUFFER_FIELD)) {
        const char* fmt = "Attribute must be BufferField, not %C";
        YogError_raise_TypeError(env, fmt, attr);
    }
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_BUFFER)) {
        YogError_raise_TypeError(env, "Object must be Buffer, not %C", val);
    }
    Struct_write_Buffer(env, obj, attr, val);
}

static void
PointerField_exec_descr_set(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    if (!IS_PTR(attr) || (BASIC_OBJ_TYPE(attr) != TYPE_POINTER_FIELD)) {
        const char* fmt = "Attribute must be PointerField, not %C";
        YogError_raise_TypeError(env, fmt, attr);
    }
    YogVal klass = YogVal_get_class(env, val);
    if (klass != PTR_AS(PointerField, attr)->klass) {
        ID name = PTR_AS(YogClass, klass)->name;
        const char* fmt = "Field value must be %I, not %C";
        YogError_raise_TypeError(env, fmt, name, val);
    }
    Struct_write_pointer(env, obj, attr, val);
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
    if (write_data(env, PTR_AS(Struct, self)->data + offset, type, val)) {
        return;
    }
    YogError_raise_ValueError(env, "unknown type - %I", type);
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
Pointer_get_value(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_value", params, args, kw);
    CHECK_SELF_POINTER;
    void* ptr = PTR_AS(Pointer, self)->ptr;
    RETURN(env, YogVal_from_unsigned_int(env, (uint_t)ptr));
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

static YogVal
UnionClass_define_fields(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal fields = YUNDEF;
    PUSH_LOCAL(env, fields);
    YogCArg params[] = { { "fields", &fields }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "define_fields", params, args, kw);
    CHECK_SELF_UNION_CLASS;

    define_fields_to_UnionClass(env, self, fields);

    RETURN(env, self);
}

static YogVal
UnionClassClass_new(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);

    YogVM* vm = env->vm;
    YogVal cUnionClass = YogClass_new(env, "UnionClass", vm->cClass);
    YogVal klass = vm->cUnionClassClass;
    YogGC_UPDATE_PTR(env, PTR_AS(YogBasicObj, cUnionClass), klass, klass);
    YogClass_define_method(env, cUnionClass, pkg, "define_fields", UnionClass_define_fields);

    RETURN(env, cUnionClass);
}

static YogVal
StructClassClass_new(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);

    YogVM* vm = env->vm;
    YogVal cStructClass = YogClass_new(env, "StructClass", vm->cClass);
    YogVal klass = vm->cStructClassClass;
    YogGC_UPDATE_PTR(env, PTR_AS(YogBasicObj, cStructClass), klass, klass);
    YogClass_define_method(env, cStructClass, pkg, "define_fields", StructClass_define_fields);

    RETURN(env, cStructClass);
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
    YogVal cPointerField = YUNDEF;
    YogVal cStringField = YUNDEF;
    YogVal cStructBase = YUNDEF;
    YogVal cStructClassClass = YUNDEF;
    YogVal cStructField = YUNDEF;
    YogVal cUnionClassClass = YUNDEF;
    PUSH_LOCALS8(env, cUnionClassClass, cLib, cLibFunc, cStructClassClass, cField, cInt, cBuffer, cPointer);
    PUSH_LOCALS7(env, cArrayField, cPointerField, cStringField, cFieldArray, cStructField, cStructBase, cBufferField);
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
    YogClass_define_method(env, cStructClassClass, pkg, "new", StructClass_new);
    vm->cStructClassClass = cStructClassClass;
    cStructBase = YogClass_new(env, "StructBase", vm->cClass);
    YogClass_define_property(env, cStructBase, pkg, "size", StructBase_get_size, NULL);
    YogClass_define_allocator(env, cStructBase, StructBase_alloc);
    YogClass_define_method(env, cStructBase, pkg, "init", StructBase_init);
    vm->cStructBase = cStructBase;
    vm->cStructClass = StructClassClass_new(env, pkg);
    cUnionClassClass = YogClass_new(env, "UnionClassClass", vm->cClass);
    YogClass_define_method(env, cUnionClassClass, pkg, "new", UnionClass_new);
    vm->cUnionClassClass = cUnionClassClass;
    vm->cUnionClass = UnionClassClass_new(env, pkg);
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
    cPointerField = YogClass_new(env, "PointerField", vm->cObject);
    YogClass_define_allocator(env, cPointerField, PointerField_alloc);
    YogClass_define_descr_get_executor(env, cPointerField, PointerField_exec_descr_get);
    YogClass_define_descr_get_caller(env, cPointerField, PointerField_call_descr_get);
    YogClass_define_descr_set_executor(env, cPointerField, PointerField_exec_descr_set);
    vm->cPointerField = cPointerField;
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
    YogClass_define_property(env, cPointer, pkg, "value", Pointer_get_value, NULL);
    vm->cPointer = cPointer;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
