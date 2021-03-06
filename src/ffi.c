#include "yog/config.h"
#include <ctype.h>
#if defined(YOG_HAVE_STDLIB_H)
#   include <stdlib.h>
#endif
#if defined(YOG_HAVE_STRING_H)
#   include <string.h>
#endif
#if defined(YOG_HAVE_STRINGS_H)
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
    void* handle;
};

typedef struct Lib Lib;

#define TYPE_LIB TO_TYPE(Lib_alloc)

struct LibFunc {
    struct YogBasicObj base;
    ffi_cif cif;
    void* f;
    YogVal rtype;
    uint_t nargs;
    YogVal nodes[0];
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

struct BitField {
    struct FieldBase base;
    uint_t pos;
    uint_t width;
};

typedef struct BitField BitField;

#define TYPE_BIT_FIELD TO_TYPE(BitField_alloc)

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
    ffi_type* type;
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
    NODE_BIT,
    NODE_BUFFER,
    NODE_FIELD,
    NODE_POINTER,
    NODE_STRING,
    NODE_STRUCT,
};

typedef enum NodeType NodeType;

enum BitSign {
    BIT_SIGNED,
    BIT_UNSIGNED,
};

typedef enum BitSign BitSign;

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
            BitSign sign;
            uint_t width;
        } bit_field;
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
    case NODE_BIT:
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

static ID
intern_pointer(YogEnv* env)
{
    return YogVM_intern(env, env->vm, "pointer");
}

static YogVal
create_atom_node(YogEnv* env, ID type)
{
    YogVal node = Node_new(env);
    PTR_AS(Node, node)->type = NODE_ATOM;
    PTR_AS(Node, node)->u.atom.type = type;
    return node;
}

static YogVal
create_pointer_node(YogEnv* env)
{
    return create_atom_node(env, intern_pointer(env));
}

static YogVal
PointerField_new(YogEnv* env, uint_t offset, YogVal klass, uint_t child_index)
{
    SAVE_ARG(env, klass);
    YogVal field = YUNDEF;
    PUSH_LOCAL(env, field);
    if (!IS_PTR(klass) || (BASIC_OBJ_TYPE(klass) != TYPE_STRUCT_CLASS)) {
        return create_pointer_node(env);
    }

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
BitField_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal field = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, BitField);
    PUSH_LOCAL(env, field);
    FieldBase_init(env, field, TYPE_BIT_FIELD, klass);
    PTR_AS(BitField, field)->pos = 0;
    PTR_AS(BitField, field)->width = 0;
    RETURN(env, field);
}

#define BITS_PER_BYTE   8
#define BITS_NUM        (BITS_PER_BYTE * sizeof(void*))

static YogVal
alloc_bit_field(YogEnv* env, YogVal klass, uint_t offset, uint_t pos, uint_t width)
{
    if (width == 0) {
        YogError_raise_ValueError(env, "Zero width for bit field");
    }
    if (BITS_NUM < width) {
        YogError_raise_ValueError(env, "Width of bit field exceeds its type");
    }

    YogVal field = BitField_alloc(env, klass);
    /**
     * FIXME: This file includes the following
     * PTR_AS(Foo, bar)->offset = offset at many places.
     */
    PTR_AS(FieldBase, field)->offset = offset;
    PTR_AS(BitField, field)->pos = pos;
    PTR_AS(BitField, field)->width = width;
    return field;
}

static YogVal
UnsignedBitField_new(YogEnv* env, uint_t offset, uint_t pos, uint_t width)
{
    return alloc_bit_field(env, env->vm->cUnsignedBitField, offset, pos, width);
}

static YogVal
BitField_new(YogEnv* env, uint_t offset, uint_t pos, uint_t width)
{
    return alloc_bit_field(env, env->vm->cBitField, offset, pos, width);
}

static YogVal
create_bit_field(YogEnv* env, uint_t offset, uint_t bit_pos, BitSign sign, uint_t width)
{
    YogVal (*f)(YogEnv*, uint_t, uint_t, uint_t);
    f = sign == BIT_SIGNED ? BitField_new : UnsignedBitField_new;
    return f(env, offset, bit_pos, width);
}

static YogVal
create_field(YogEnv* env, YogVal node, uint_t offset, uint_t bit_pos, uint_t child_index)
{
    YogVal klass;
    YogVal encoding;
    BitSign sign;
    uint_t width;
    switch (PTR_AS(Node, node)->type) {
    case NODE_ARRAY:
        return ArrayField_new(env, node, offset);
    case NODE_ATOM:
        return Field_new(env, offset, PTR_AS(Node, node)->u.atom.type);
    case NODE_BIT:
        sign = PTR_AS(Node, node)->u.bit_field.sign;
        width = PTR_AS(Node, node)->u.bit_field.width;
        return create_bit_field(env, offset, bit_pos, sign, width);
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
StructClass_set_field(YogEnv* env, YogVal self, YogVal node, uint_t offset, uint_t bit_pos, uint_t child_index)
{
    SAVE_ARGS2(env, self, node);

    ID name = PTR_AS(Node, node)->u.field.name;
    YogVal type = PTR_AS(Node, node)->u.field.type;
    YogVal field = create_field(env, type, offset, bit_pos, child_index);
    YogObj_set_attr_id(env, self, name, field);

    RETURN_VOID(env);
}

static void
init_struct(YogEnv* env, YogVal obj, YogVal klass, ID name)
{
    YogClass_init(env, obj, TYPE_STRUCT_CLASS, klass);
    PTR_AS(YogClass, obj)->name = name;

    PTR_AS(StructClass, obj)->size = 0;
    PTR_AS(StructClass, obj)->type = NULL;
    YogGC_UPDATE_PTR(env, PTR_AS(YogClass, obj), super, env->vm->cStructBase);
}

static void
UnionClass_init(YogEnv* env, YogVal self, ID name)
{
    init_struct(env, self, env->vm->cUnionClass, name);
}

static void
StructClass_init(YogEnv* env, YogVal self, ID name)
{
    init_struct(env, self, env->vm->cStructClass, name);
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
         * gcc complains "'size' may be used uninitialized in this function"
         * without the following assignment.
         */
        size = 0;
    }

    return size;
}

static uint_t
type2size(YogEnv* env, ffi_type* type, YogVal node)
{
    SAVE_ARG(env, node);

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
    else if (PTR_AS(Node, node)->type == NODE_STRUCT) {
        YogVal klass = PTR_AS(Node, node)->u.struct_.klass;
        size = PTR_AS(StructClass, klass)->size;
    }
    else {
        YogError_raise_ValueError(env, "Unknown FFI type");
        /* NOTREACHED */
        /**
         * gcc complains "'size' may be used uninitialized in this function"
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
    NodeType type = PTR_AS(Node, node)->type;
    switch (type) {
    case NODE_ATOM:
    case NODE_BUFFER:
    case NODE_POINTER:
    case NODE_STRING:
        return get_size(env, node);
    case NODE_STRUCT:
        klass = PTR_AS(Node, node)->u.struct_.klass;
        return PTR_AS(StructClass, klass)->alignment_unit;
    case NODE_BIT:
        return 1;
    case NODE_ARRAY:
    case NODE_FIELD:
    default:
        YOG_BUG(env, "Invalid Node type (%u)", type);
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

#define ALIGN(offset, alignment) \
                    (((offset) + (alignment) - 1) & ~((alignment) - 1))
static uint_t
compute_bit_field_size(YogEnv* env, uint_t pos)
{
    return ALIGN(pos, BITS_PER_BYTE) / BITS_PER_BYTE;
}

static void
align_offset(YogEnv* env, YogVal node, uint_t* offset, uint_t* bit_pos)
{
    SAVE_ARG(env, node);

    YogVal type = PTR_AS(Node, node)->u.field.type;
    if (PTR_AS(Node, type)->type != NODE_BIT) {
        uint_t size = get_alignment_unit(env, node);
        uint_t alignment = sizeof(void*) < size ? sizeof(void*) : size;
        uint_t bit_field_size = compute_bit_field_size(env, *bit_pos);
        *offset = ALIGN(*offset + bit_field_size, alignment);
        *bit_pos = 0;
        RETURN_VOID(env);
    }

    if (BITS_NUM <= *bit_pos + PTR_AS(Node, type)->u.bit_field.width - 1) {
        *offset += sizeof(void*);
        *bit_pos = 0;
        RETURN_VOID(env);
    }

    RETURN_VOID(env);
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

static void
dump_data(YogEnv* env, YogVal s, char* ptr, const char* prefix, uint_t size)
{
    SAVE_ARG(env, s);
    if (size == 0) {
        RETURN_VOID(env);
    }
    YogString_append_string(env, s, prefix);
#define FMT "0x%02x"
    char buf[strlen(FMT) + 1];
    snprintf(buf, array_sizeof(buf), FMT, 0xff & *ptr);
#undef FMT
    YogString_append_string(env, s, buf);
    dump_data(env, s, ptr + 1, " ", size - 1);
    RETURN_VOID(env);
}

static YogVal
StructBase_dump(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YogString_new(env);
    PUSH_LOCAL(env, s);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "dump", params, args, kw);
    CHECK_SELF_STRUCT;

    YogVal klass = YogVal_get_class(env, self);
    CHECK_STRUCT_CLASS(env, klass, "Class");
    uint_t size = PTR_AS(StructClass, klass)->size;
    dump_data(env, s, PTR_AS(Struct, self)->data, "", size);

    RETURN(env, s);
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
    CHECK_SELF_STRUCT;

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
        RETURN(env, create_pointer_node(env));
    }
    node = Node_new(env);
    PTR_AS(Node, node)->type = NODE_POINTER;
    YogGC_UPDATE_PTR(env, PTR_AS(Node, node), u.pointer.klass, subinfo);
    RETURN(env, node);
}

static YogVal
create_bit_node(YogEnv* env, YogVal width, BitSign sign)
{
    SAVE_ARG(env, width);
    YogVal node = Node_new(env);
    PUSH_LOCAL(env, node);
    if (!IS_FIXNUM(width)) {
        const char* fmt = "Bit-field width must be Fixnum, not %C";
        YogError_raise_TypeError(env, fmt, width);
    }

    PTR_AS(Node, node)->type = NODE_BIT;
    PTR_AS(Node, node)->u.bit_field.sign = sign;
    PTR_AS(Node, node)->u.bit_field.width = VAL2INT(width);
    RETURN(env, node);
}

static YogVal
parse_subtype(YogEnv* env, YogVal type)
{
    SAVE_ARG(env, type);
    YogVal node = YUNDEF;
    YogVal subinfo = YUNDEF;
    PUSH_LOCALS2(env, node, subinfo);

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
#define INTERN(name)    YogVM_intern(env, vm, (name))
    if (sym == INTERN("struct")) {
        RETURN(env, create_anonymous_struct_node(env, type, build_StructClass));
    }
    if (sym == INTERN("union")) {
        RETURN(env, create_anonymous_struct_node(env, type, build_UnionClass));
    }
    subinfo = YogArray_at(env, type, 1);
    if (sym == INTERN("pointer")) {
        RETURN(env, parse_pointer(env, subinfo));
    }
    if (sym == INTERN("bit")) {
        RETURN(env, create_bit_node(env, subinfo, BIT_SIGNED));
    }
    if (sym == INTERN("ubit")) {
        RETURN(env, create_bit_node(env, subinfo, BIT_UNSIGNED));
    }
#undef INTERN
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
    uint_t size;
    switch (PTR_AS(Node, type)->type) {
    case NODE_ARRAY:
        size = PTR_AS(Node, type)->u.array.size;
        return get_size(env, PTR_AS(Node, type)->u.array.type) * size;
    case NODE_BIT:
        return 0;
    default:
        return get_size(env, type);
    }
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

static void
StructClass_finalize(YogEnv* env, void* ptr)
{
    StructClass* klass = (StructClass*)ptr;
    free(klass->type);
}

static YogVal
allocate_StructClass(YogEnv* env)
{
    return ALLOC_OBJ(env, YogClass_keep_children, StructClass_finalize, StructClass);
}

static YogVal
create_UnionClass(YogEnv* env, ID name)
{
    YogVal obj = allocate_StructClass(env);
    UnionClass_init(env, obj, name);
    return obj;
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
         * gcc complains "'cif_type' may be used uninitialized in this function"
         * without the following assignment.
         */
        cif_type = NULL;
    }

    return cif_type;
}

static ffi_type*
map_type(YogEnv* env, YogVal node)
{
    YogVal klass;
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
        klass = PTR_AS(Node, node)->u.struct_.klass;
        return PTR_AS(StructClass, klass)->type;
    case NODE_BIT:
    case NODE_FIELD:
    default:
        YOG_BUG(env, "Invalid Node type (%u)", PTR_AS(Node, node)->type);
    }

    return NULL;
}

static ffi_type*
map_type_of_fields(YogEnv* env, YogVal node)
{
    NodeType type = PTR_AS(Node, node)->type;
    if ((type == NODE_ARRAY) || (type == NODE_BIT)) {
        return NULL;
    }
    return map_type(env, node);
}

static void
update_union_elements(YogEnv* env, YogVal node, ffi_type** elements, uint_t* max_size)
{
    SAVE_ARG(env, node);
    YogVal type = PTR_AS(Node, node)->u.field.type;
    PUSH_LOCAL(env, type);

    uint_t field_size = get_size(env, type);
    if (field_size <= *max_size) {
        RETURN_VOID(env);
    }
    *elements = map_type_of_fields(env, type);
    *max_size = field_size;
    RETURN_VOID(env);
}

static ffi_type*
allocate_ffi_type(YogEnv* env, ffi_type** elements, uint_t size)
{
    ffi_type* type = (ffi_type*)YogGC_malloc(env, sizeof(ffi_type) + size);
    type->size = 0;
    type->alignment = 0;
    type->type = FFI_TYPE_STRUCT;
    type->elements = (ffi_type**)(type + 1);
    memcpy(type->elements, elements, size);
    return type;
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
    uint_t elements_num = YogArray_size(env, fields) == 0 ? 0 : 1;
    uint_t size = sizeof(ffi_type*) * (elements_num + 1);
    ffi_type** elements = (ffi_type**)alloca(size);
    uint_t max_size = 0;
    for (node = nodes; IS_PTR(node); node = PTR_AS(Node, node)->u.field.next) {
        StructClass_set_field(env, self, node, 0, 0, children_num);
        children_num += get_children_num(env, node);
        update_union_elements(env, node, elements, &max_size);
    }
    elements[elements_num] = NULL;
    uint_t union_size = get_union_size(env, nodes);
    PTR_AS(StructClass, self)->size = union_size;
    PTR_AS(StructClass, self)->children_num = children_num;
    PTR_AS(StructClass, self)->type = allocate_ffi_type(env, elements, size);
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
    YogVal obj = allocate_StructClass(env);
    StructClass_init(env, obj, name);
    return obj;
}

static void
define_fields_to_StructClass(YogEnv* env, YogVal self, YogVal fields)
{
    SAVE_ARGS2(env, self, fields);
    YogVal nodes = parse_fields(env, fields);
    YogVal node = YUNDEF;
    PUSH_LOCALS2(env, nodes, node);

    /**
     * BUG: The following code is a bug. See
     * issues/issue-6bbc1f7eee57d44175359d68abdd109bf826dddf.yaml.
     * Alignment of struct must be decided by the biggest field.
     */
    uint_t alignment_unit = get_alignment_unit(env, nodes);
    PTR_AS(StructClass, self)->alignment_unit = alignment_unit;

    uint_t offset = 0;
    uint_t bit_pos = 0;
    uint_t children_num = 0;
    uint_t size = sizeof(ffi_type*) * (YogArray_size(env, fields) + 1);
    ffi_type** elements = (ffi_type**)alloca(size);
    uint_t i = 0;
    for (node = nodes; IS_PTR(node); node = PTR_AS(Node, node)->u.field.next) {
        align_offset(env, node, &offset, &bit_pos);
        StructClass_set_field(env, self, node, offset, bit_pos, children_num);
        offset += get_total_field_size(env, node);
        YogVal tn = PTR_AS(Node, node)->u.field.type;
        NodeType type = PTR_AS(Node, tn)->type;
        bit_pos += type == NODE_BIT ? PTR_AS(Node, tn)->u.bit_field.width : 0;
        children_num += get_children_num(env, node);
        elements[i] = map_type_of_fields(env, tn);
        i++;
    }
    elements[i] = NULL;

    uint_t bit_field_size = compute_bit_field_size(env, bit_pos);
    PTR_AS(StructClass, self)->size = offset + bit_field_size;
    PTR_AS(StructClass, self)->children_num = children_num;
    PTR_AS(StructClass, self)->type = allocate_ffi_type(env, elements, size);

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
        RETURN(env, create_UnionClass(env, id));
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
        RETURN(env, create_StructClass(env, id));
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
        YogGC_KEEP(env, f, nodes[i], keeper, heap);
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
        PTR_AS(LibFunc, obj)->nodes[i] = YUNDEF;
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
    void* handle = YogMisc_load_lib(env, path);
    if (handle == NULL) {
        const char* fmt = "No library named \"%S\"";
        YogError_raise_ImportError(env, fmt, HDL2VAL(path));
        /* NOTREACHED */
    }
    YogVal lib = Lib_alloc(env, env->vm->cLib);
    PTR_AS(Lib, lib)->handle = handle;

    return lib;
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
        YogGC_UPDATE_PTR(env, PTR_AS(LibFunc, f), nodes[i], node);
    }
    node = rtype2node(env, rtype);
    YogGC_UPDATE_PTR(env, PTR_AS(LibFunc, f), rtype, node);
    ffi_abi abi = FFI_DEFAULT_ABI;
    ffi_type* ffi_rtype = IS_NIL(node) ? &ffi_type_void : map_type(env, node);
    ffi_cif* cif = &PTR_AS(LibFunc, f)->cif;
    ffi_status status = ffi_prep_cif(cif, abi, nargs, ffi_rtype, types);
    if (status != FFI_OK) {
        YogError_raise_FFIError(env, "%s", map_ffi_error(env, status));
        /* NOTREACHED */
    }
    YogVal s = YogString_to_bin_in_default_encoding(env, VAL2HDL(env, name));
    void* p = dlsym(PTR_AS(Lib, self)->handle, BINARY_CSTR(s));
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
check_Fixnum_int32(YogEnv* env, YogVal val)
{
    int_t n = VAL2INT(val);
    if (n < INT32_MIN) {
        const char* fmt = "Value must be greater or equal %d, not %D";
        YogError_raise_ValueError(env, fmt, INT32_MIN, val);
    }
    if (INT32_MAX < n) {
        const char* fmt = "Value must be less or equal %d, not %D";
        YogError_raise_ValueError(env, fmt, INT32_MAX, val);
    }
}

static void
check_Fixnum_uint32(YogEnv* env, YogVal val)
{
    int_t n = VAL2INT(val);
    if (n < 0) {
        const char* fmt = "Value must be greater or equal 0, not %D";
        YogError_raise_ValueError(env, fmt, val);
    }
    if (UINT32_MAX < n) {
        const char* fmt = "Value must be less or equal %u, not %D";
        YogError_raise_ValueError(env, fmt, UINT32_MAX, val);
    }
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
    SAVE_ARG(env, bignum);

    if (0 <= YogBignum_compare_with_long_long(env, bignum, n)) {
        RETURN_VOID(env);
    }
    const char* fmt = "Value must be greater or equal %lld, not %D";
    YogError_raise_ValueError(env, fmt, n, bignum);

    RETURN_VOID(env);
}

static void
check_Bignum_is_less_or_equal_than_long_long(YogEnv* env, YogVal bignum, long long n)
{
    SAVE_ARG(env, bignum);

    if (YogBignum_compare_with_long_long(env, bignum, INT64_MAX) <= 0) {
        RETURN_VOID(env);
    }
    const char* fmt = "Value must be less or equal %llu, not %D";
    YogError_raise_ValueError(env, fmt, n, bignum);

    RETURN_VOID(env);
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
        check_Fixnum_uint32(env, val);
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
        check_Fixnum_int32(env, val);
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
raise_TypeError_for_struct(YogEnv* env, YogVal expected, YogVal actual)
{
    ID name = PTR_AS(YogClass, expected)->name;
    YogError_raise_TypeError(env, "Argument must be %I, not %C", name, actual);
}

static void
check_Struct(YogEnv* env, YogVal val, YogVal arg_type)
{
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_STRUCT)) {
        raise_TypeError_for_struct(env, arg_type, val);
    }
    YogVal klass = YogVal_get_class(env, val);
    if (klass != arg_type) {
        raise_TypeError_for_struct(env, arg_type, val);
    }
}

static void
write_argument_struct_pointer(YogEnv* env, void* ptr, YogVal arg_type, YogVal val)
{
    check_Struct(env, val, arg_type);
    *((void**)ptr) = PTR_AS(Struct, val)->data;
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
        check_Bignum_uint32(env, val);
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
    if (IS_NIL(val)) {
        *((void**)pvalue) = NULL;
        return;
    }
    if (IS_PTR(klass) && (BASIC_OBJ_TYPE(klass) == TYPE_STRUCT_CLASS)) {
        write_argument_struct_pointer(env, pvalue, klass, val);
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
write_argument_string(YogEnv* env, void* pvalue, void* refered, YogVal encoding, YogVal val)
{
    SAVE_ARGS2(env, encoding, val);
    if (IS_NIL(val)) {
        *((char**)pvalue) = NULL;
        RETURN_VOID(env);
    }

    YogHandle* h = VAL2HDL(env, val);
    YogMisc_check_String(env, h, "Actual parameter");
    YogVal bin = YogEncoding_conv_from_yog(env, VAL2HDL(env, encoding), h);
    memcpy(refered, BINARY_CSTR(bin), BINARY_SIZE(bin));
    *((char**)pvalue) = (char*)refered;

    RETURN_VOID(env);
}

static void
write_argument_struct(YogEnv* env, void* pvalue, YogVal klass, YogVal val)
{
    check_Struct(env, val, klass);
    void* data = PTR_AS(Struct, val)->data;
    memcpy(pvalue, data, PTR_AS(StructClass, klass)->size);
}

static void
write_argument(YogEnv* env, void* pvalue, void* refered, YogVal node, YogVal val)
{
    SAVE_ARGS2(env, node, val);
    YogVal encoding;
    YogVal klass;
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
        encoding = PTR_AS(Node, node)->u.string.encoding;
        write_argument_string(env, pvalue, refered, encoding, val);
        RETURN_VOID(env);
    case NODE_STRUCT:
        klass = PTR_AS(Node, node)->u.struct_.klass;
        write_argument_struct(env, pvalue, klass, val);
        RETURN_VOID(env);
    case NODE_ARRAY:
    case NODE_FIELD:
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
    if (IS_NIL(arg)) {
        return 0;
    }
    YogVal enc = PTR_AS(Node, node)->u.string.encoding;
    YogMisc_check_String(env, VAL2HDL(env, arg), "Argument");
    return PTR_AS(YogEncoding, enc)->max_size * STRING_SIZE(arg) + 1;
}

static uint_t
type2refered_size(YogEnv* env, YogVal node, YogVal arg)
{
    switch (PTR_AS(Node, node)->type) {
    case NODE_ATOM:
        break;
    case NODE_BUFFER:
    case NODE_POINTER:
    case NODE_STRUCT:
        return 0;
    case NODE_STRING:
        return type2refered_size_of_string(env, node, arg);
    case NODE_ARRAY:
    case NODE_FIELD:
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
create_struct_from_pointer(YogEnv* env, YogVal klass, void* ptr)
{
    YogVal obj = StructBase_alloc(env, klass);
    init_struct_with_ptr(env, obj, ptr);
    return obj;
}

static YogVal
create_ptr_retval(YogEnv* env, YogHandle* callee, void* rvalue)
{
    YogVal node = HDL_AS(LibFunc, callee)->rtype;
    NodeType type = PTR_AS(Node, node)->type;
    if (type == NODE_ATOM) {
        return ptr2int_retval(env, node, rvalue);
    }
    if (rvalue == NULL) {
        return YNIL;
    }
    if (type == NODE_STRING) {
        return YogString_from_string(env, (char*)rvalue);
    }
    if (type != NODE_POINTER) {
        const char* s = "Node::type must be NODE_ATOM or NODE_POINTER, not %s";
        YogError_raise_FFIError(env, s, NodeType_to_s(env, type));
    }
    YogVal klass = PTR_AS(Node, node)->u.pointer.klass;
    if (IS_PTR(klass) && (BASIC_OBJ_TYPE(klass) == TYPE_STRUCT_CLASS)) {
        return create_struct_from_pointer(env, klass, rvalue);
    }
    const char* fmt = "rtype must be a pointer to Struct or void, not %C";
    YogError_raise_TypeError(env, fmt, klass);
    return YUNDEF;
}

static uint_t
compute_args_num(YogEnv* env, uint8_t posargc, YogHandle* vararg)
{
    if (vararg == NULL) {
        return posargc;
    }
    YogVal val = HDL2VAL(vararg);
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_ARRAY)) {
        YogError_raise_TypeError_for_vararg(env, val);
    }
    return posargc + YogArray_size(env, val);
}

static YogVal
get_posarg_at(YogEnv* env, uint_t i, uint8_t posargc, YogHandle* posargs[], YogHandle* vararg)
{
    if (i < posargc) {
        return HDL2VAL(posargs[i]);
    }
    return YogArray_at(env, HDL2VAL(vararg), i - posargc);
}

static YogVal
LibFunc_do(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    uint_t nargs = HDL_AS(LibFunc, callee)->nargs;
    if (compute_args_num(env, posargc, vararg) != nargs) {
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
        YogVal val = get_posarg_at(env, i, posargc, posargs, vararg);
        YogHandle* h_val = VAL2HDL(env, val);
        YogVal node = HDL_AS(LibFunc, callee)->nodes[i];
        YogHandle* h_node = VAL2HDL(env, node);
        uint_t size = type2size(env, ffi_arg_type, node);
        void* pvalue = YogSysdeps_alloca(size);
        uint_t refered_size = type2refered_size(env, node, val);
        void* refered = 0 < refered_size ? YogSysdeps_alloca(refered_size): NULL;
        write_argument(env, pvalue, refered, HDL2VAL(h_node), HDL2VAL(h_val));
        values[i] = pvalue;
        refereds[i] = refered;
    }

    ffi_type* rtype = HDL_AS(LibFunc, callee)->cif.rtype;
    void* rvalue = rtype != &ffi_type_void ? YogSysdeps_alloca(type2size(env, rtype, PTR_AS(LibFunc, callee)->rtype)) : NULL;

    ffi_call(&HDL_AS(LibFunc, callee)->cif, HDL_AS(LibFunc, callee)->f, rvalue, values);

    for (i = 0; i < nargs; i++) {
        YogVal node = HDL_AS(LibFunc, callee)->nodes[i];
        YogVal val = get_posarg_at(env, i, posargc, posargs, vararg);
        read_argument(env, val, node, refereds[i]);
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
    return PTR_AS(Struct, self)->children[index];
}

static YogVal
Struct_get_Buffer(YogEnv* env, YogVal self, YogVal field)
{
    if (!IS_PTR(field) || (BASIC_OBJ_TYPE(field) != TYPE_BUFFER_FIELD)) {
        const char* fmt = "Attribute must be BufferField, not %C";
        YogError_raise_TypeError(env, fmt, field);
    }
    CHECK_SELF_STRUCT;

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
    CHECK_SELF_STRUCT;
    if (!PTR_AS(Struct, self)->own) {
        YogVal klass = PTR_AS(PointerField, field)->klass;
        uint_t offset = PTR_AS(FieldBase, field)->offset;
        void* ptr = *((void**)((char*)PTR_AS(Struct, self)->data + offset));
        return create_struct_from_pointer(env, klass, ptr);
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
         * gcc complains "'val' may be used uninitialized in this function"
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
make_meta_class(YogEnv* env, YogVal pkg, const char* name, YogVal klass, YogAPI define_fields)
{
    SAVE_ARGS2(env, pkg, klass);
    YogVM* vm = env->vm;
    YogVal obj = YogClass_new(env, name, vm->cClass);
    PUSH_LOCAL(env, obj);

    YogGC_UPDATE_PTR(env, PTR_AS(YogBasicObj, obj), klass, klass);
    YogClass_define_method(env, obj, pkg, "define_fields", define_fields);

    RETURN(env, obj);
}

static YogVal
UnionClassClass_new(YogEnv* env, YogVal pkg)
{
    return make_meta_class(env, pkg, "UnionClass", env->vm->cUnionClassClass, UnionClass_define_fields);
}

static YogVal
StructClassClass_new(YogEnv* env, YogVal pkg)
{
    return make_meta_class(env, pkg, "StructClass", env->vm->cStructClassClass, StructClass_define_fields);
}

static uint_t
make_mask(uint_t width)
{
    return width < BITS_NUM ? (1 << width) - 1 : ~0;
}

static int_t
extend_sign(YogEnv* env, int_t n, uint_t width)
{
    if (((n >> (width - 1)) & 1) == 1) {
        return n | ~make_mask(width);
    }
    return n;
}

#define STRUCT_DATA(self, field) \
    ((char*)PTR_AS(Struct, (self))->data + PTR_AS(FieldBase, (field))->offset)

static YogVal
BitField_call_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    /* FIXME: Validate attr, obj and klass */
    /**
     * TODO: The following three lines are similar with lines in
     * UnsignedBitField_call_descr_get. Make a function or a macro to share.
     */
    uint_t pos = PTR_AS(BitField, attr)->pos;
    uint_t width = PTR_AS(BitField, attr)->width;
    int_t n = (*((int_t*)STRUCT_DATA(obj, attr)) >> pos) & make_mask(width);
    return YogVal_from_int(env, extend_sign(env, n, width));
}

static void
BitField_exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    YogVal val = BitField_call_descr_get(env, attr, obj, klass);
    YogScriptFrame_push_stack(env, env->frame, val);
}

static uint_t
min(uint_t n, uint_t m)
{
    return n < m ? n : m;
}

static void
write_bit_field(YogEnv* env, char* ptr, uint_t n, uint_t pos, uint_t width)
{
    char* next_ptr = ptr + 1;
    if (BITS_PER_BYTE <= pos) {
        write_bit_field(env, next_ptr, n, pos - BITS_PER_BYTE, width);
        return;
    }

    uint_t upper_mask = ~make_mask(min(width, BITS_PER_BYTE - pos)) << pos;
    uint_t lower_mask = make_mask(pos);
    uint_t mask = upper_mask | lower_mask;
    *ptr = (mask & *ptr) | ((0xff & n) << pos);
    if (width <= BITS_PER_BYTE) {
        return;
    }
    uint_t next_n = n >> BITS_PER_BYTE;
    write_bit_field(env, next_ptr, next_n, 0, width - BITS_PER_BYTE);
}

static void
BitField_exec_descr_set(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    /* FIXME: Validate attr, obj */
    int_t n = YogVal_to_signed_type(env, val, "Value");
    /**
     * If a bit-field is signed, the most significant bit is for sign. But GCC
     * doesn't set this flag.
     *
     * $ cat bar.c
     * #include <stdio.h>
     * #include <strings.h>
     *
     * int
     * main(int argc, const char* argv[])
     * {
     *     struct {
     *         signed int bar: 8;
     *     } foo;
     *     bzero(&foo, sizeof(foo));
     *     foo.bar = atoi(argv[1]);
     *     printf("%u", *((char*)&foo));
     *
     *     return 0;
     * }
     * $ gcc bar.c
     * $ ./a.out -2147483648
     * 0
     * $ ./a.out -2147483647
     * 1
     */
    uint_t pos = PTR_AS(BitField, attr)->pos;
    uint_t width = PTR_AS(BitField, attr)->width;
    int_t max = make_mask(width - 1);
    if (max < n) {
        const char* fmt = "Value must be less or equal %d, not %d";
        YogError_raise_ValueError(env, fmt, max, n);
    }
    int_t min = ~max;
    if (n < min) {
        const char* fmt = "Value must be greater or equal %d, not %d";
        YogError_raise_ValueError(env, fmt, min, n);
    }
    write_bit_field(env, STRUCT_DATA(obj, attr), (uint_t)n, pos, width);
}

static YogVal
UnsignedBitField_call_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    /* FIXME: Validate attr, obj and klass */
    uint_t pos = PTR_AS(BitField, attr)->pos;
    uint_t width = PTR_AS(BitField, attr)->width;
    uint_t n = *((uint_t*)STRUCT_DATA(obj, attr));
    return YogVal_from_unsigned_int(env, (n >> pos) & make_mask(width));
}

static void
UnsignedBitField_exec_descr_get(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    YogVal val = UnsignedBitField_call_descr_get(env, attr, obj, klass);
    YogScriptFrame_push_stack(env, env->frame, val);
}

static void
UnsignedBitField_exec_descr_set(YogEnv* env, YogVal attr, YogVal obj, YogVal val)
{
    /* FIXME: Validate attr, obj */
    uint_t n = YogVal_to_uint(env, val, "Value");
    uint_t pos = PTR_AS(BitField, attr)->pos;
    uint_t width = PTR_AS(BitField, attr)->width;
    uint_t max = make_mask(width);
    if (max < n) {
        const char* fmt = "Value must be less or equal %u, not %u";
        YogError_raise_ValueError(env, fmt, max, n);
    }
    write_bit_field(env, STRUCT_DATA(obj, attr), n, pos, width);
}

void
YogFFI_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cArrayField = YUNDEF;
    YogVal cBitField = YUNDEF;
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
    YogVal cUnsignedBitField = YUNDEF;
    PUSH_LOCALS8(env, cUnionClassClass, cLib, cLibFunc, cStructClassClass, cField, cInt, cBuffer, cPointer);
    PUSH_LOCALS8(env, cArrayField, cPointerField, cStringField, cFieldArray, cStructField, cStructBase, cBufferField, cBitField);
    PUSH_LOCAL(env, cUnsignedBitField);
    YogVM* vm = env->vm;

    cLib = YogClass_new(env, "Lib", vm->cObject);
    YogClass_define_allocator(env, cLib, Lib_alloc);
    YogClass_define_method(env, cLib, pkg, "load_func", load_func);
    vm->cLib = cLib;
    cLibFunc = YogClass_new(env, "LibFunc", vm->cObject);
    YogClass_include_module(env, cLibFunc, vm->mCallable);
    YogClass_define_caller(env, cLibFunc, LibFunc_call);
    YogClass_define_executor(env, cLibFunc, LibFunc_exec);
    vm->cLibFunc = cLibFunc;

    cStructClassClass = YogClass_new(env, "StructClassClass", vm->cClass);
    YogClass_define_method(env, cStructClassClass, pkg, "new", StructClass_new);
    vm->cStructClassClass = cStructClassClass;
    cStructBase = YogClass_new(env, "StructBase", vm->cClass);
    YogClass_define_property(env, cStructBase, pkg, "size", StructBase_get_size, NULL);
    YogClass_define_allocator(env, cStructBase, StructBase_alloc);
    YogClass_define_method(env, cStructBase, pkg, "dump", StructBase_dump);
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
    cBitField = YogClass_new(env, "BitField", vm->cObject);
    YogClass_define_descr_get_executor(env, cBitField, BitField_exec_descr_get);
    YogClass_define_descr_get_caller(env, cBitField, BitField_call_descr_get);
    YogClass_define_descr_set_executor(env, cBitField, BitField_exec_descr_set);
    vm->cBitField = cBitField;
    cUnsignedBitField = YogClass_new(env, "UnsignedBitField", vm->cObject);
    YogClass_define_descr_get_executor(env, cUnsignedBitField, UnsignedBitField_exec_descr_get);
    YogClass_define_descr_get_caller(env, cUnsignedBitField, UnsignedBitField_call_descr_get);
    YogClass_define_descr_set_executor(env, cUnsignedBitField, UnsignedBitField_exec_descr_set);
    vm->cUnsignedBitField = cUnsignedBitField;

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
