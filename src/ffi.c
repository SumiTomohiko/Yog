#include "yog/config.h"
#include <string.h>
#include "ffi.h"
#include "yog/array.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/eval.h"
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
};

typedef struct LibFunc LibFunc;

#define TYPE_LIB_FUNC TO_TYPE(LibFunc_alloc)

static YogVal
LibFunc_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, LibFunc);
    YogBasicObj_init(env, obj, TYPE_LIB_FUNC, 0, klass);
    PTR_AS(LibFunc, obj)->f = NULL;

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
YogFFI_load_lib(YogEnv* env, const char* path)
{
    SAVE_LOCALS(env);
    YogVal lib = YUNDEF;
    PUSH_LOCAL(env, lib);

    LIB_HANDLE handle = YogSysdeps_open_lib(path);
    if (handle == NULL) {
        YogError_raise_ImportError(env, "no library named \"%s\"", path);
    }
    lib = Lib_alloc(env, env->vm->cLib);
    PTR_AS(Lib, lib)->handle = handle;

    RETURN(env, lib);
}

static ffi_type*
map_type(YogEnv* env, ID type)
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
    else if (strcmp(t, "sint8") == 0) {
        cif_type = &ffi_type_sint8;
    }
    else if (strcmp(t, "uint16") == 0) {
        cif_type = &ffi_type_uint16;
    }
    else if (strcmp(t, "sint16") == 0) {
        cif_type = &ffi_type_sint16;
    }
    else if (strcmp(t, "uint32") == 0) {
        cif_type = &ffi_type_uint32;
    }
    else if (strcmp(t, "sint32") == 0) {
        cif_type = &ffi_type_sint32;
    }
    else if (strcmp(t, "uint64") == 0) {
        cif_type = &ffi_type_uint64;
    }
    else if (strcmp(t, "sint64") == 0) {
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
    else if (strcmp(t, "schar") == 0) {
        cif_type = &ffi_type_schar;
    }
    else if (strcmp(t, "sshort") == 0) {
        cif_type = &ffi_type_sshort;
    }
    else if (strcmp(t, "ushort") == 0) {
        cif_type = &ffi_type_sshort;
    }
    else if (strcmp(t, "uint") == 0) {
        cif_type = &ffi_type_uint;
    }
    else if (strcmp(t, "sint") == 0) {
        cif_type = &ffi_type_sint;
    }
    else if (strcmp(t, "ulong") == 0) {
        cif_type = &ffi_type_ulong;
    }
    else if (strcmp(t, "slong") == 0) {
        cif_type = &ffi_type_slong;
    }
    else if (strcmp(t, "longdouble") == 0) {
        cif_type = &ffi_type_longdouble;
    }
    else if (strcmp(t, "pointer") == 0) {
        cif_type = &ffi_type_pointer;
    }
    else {
        YogError_raise_ValueError(env, "unknown type - %s", t);
        /* NOTREACHED */
    }

    RETURN(env, cif_type);
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
find_func(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal f = YUNDEF;
    YogVal name = YUNDEF;
    YogVal arg_types = YNIL;
    YogVal rtype = YNIL;
    PUSH_LOCALS4(env, f, name, arg_types, rtype);
    YogCArg params[] = {
        { "name", &name },
        { "|", NULL },
        { "arg_types", &arg_types },
        { "rtype", &rtype },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "find_func", params, args, kw);
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

    f = LibFunc_alloc(env, env->vm->cLibFunc);
    uint_t nargs = IS_NIL(arg_types) ? 0 : YogArray_size(env, arg_types);
    ffi_type** types = (ffi_type**)YogSysdeps_alloca(sizeof(ffi_type*) * nargs);
    uint_t i;
    for (i = 0; i < nargs; i++) {
        types[i] = map_type(env, VAL2ID(YogArray_at(env, arg_types, i)));
    }
    ffi_status status = ffi_prep_cif(&PTR_AS(LibFunc, f)->cif, FFI_DEFAULT_ABI, nargs, IS_NIL(rtype) ? &ffi_type_void : map_type(env, VAL2ID(rtype)), types);
    if (status != FFI_OK) {
        const char* s = map_ffi_error(env, status);
        YogError_raise_FFIError(env, "%s", s);
    }
    void* p = YogSysdeps_get_proc(PTR_AS(Lib, self)->handle, STRING_CSTR(name));
    if (p == NULL) {
        YogError_raise_FFIError(env, "Can't find procedure address");
    }
    PTR_AS(LibFunc, f)->f = p;

    RETURN(env, f);
}

static YogVal
LibFunc_do(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS4(env, callee, vararg, varkwarg, blockarg);
    int_t rvalue = 0;
    ffi_call(&PTR_AS(LibFunc, callee)->cif, PTR_AS(LibFunc, callee)->f, &rvalue, NULL);
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

void
YogFFI_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cLib = YUNDEF;
    YogVal cLibFunc = YUNDEF;
    PUSH_LOCALS2(env, cLib, cLibFunc);
    YogVM* vm = env->vm;

    cLib = YogClass_new(env, "Lib", vm->cObject);
    YogClass_define_allocator(env, cLib, Lib_alloc);
    YogClass_define_method(env, cLib, pkg, "find_func", find_func);
    vm->cLib = cLib;
    cLibFunc = YogClass_new(env, "LibFunc", vm->cObject);
    YogClass_define_allocator(env, cLibFunc, LibFunc_alloc);
    YogClass_define_caller(env, cLibFunc, LibFunc_call);
    YogClass_define_executor(env, cLibFunc, LibFunc_exec);
    vm->cLibFunc = cLibFunc;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
