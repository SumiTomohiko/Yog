
%typemap(in) int {
    $1 = VAL2INT($input);
}

%typemap(out) int {
    $result = INT2VAL($1);
}

%typemap(in) int& (int temp) {
    temp = VAL2INT($input);
    $1 = &temp;
}

%init %{
SWIGINTERN swig_module_info*
SWIG_GetModule(void* clientdata)
{
    /* Nothing to do? */
    return NULL;
}

SWIGRUNTIME void
SWIG_SetModule(void* clientdata, swig_module_info *swig_module)
{
    /* Nothing to do? */
}
%}
%init "swiginit.swg"
%init %{
extern "C" YogVal
SWIG_init(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    YogVal cShadow = YUNDEF;
    PUSH_LOCALS2(env, pkg, cShadow);

    pkg = Package_new(env);
    cShadow = YogClass_new(env, "Shadow", YNIL);
    PTR_AS(Package, pkg)->cShadow = cShadow;

    SWIG_InitializeModule(NULL);

    Wrapper* pf = functions;
    while (pf->name != NULL) {
        YogPackage_define_function(env, pkg, pf->name, (void*)pf->f);
        pf++;
    }

    RETURN(env, pkg);
}
%}

#define %set_constant(name, value)  SWIG_SetConstant(env, name, value)
#define %raise(obj, type, desc)     SWIG_Raise(obj, type, desc); SWIG_fail
#define %append_output(obj)     $result = SWIG_AppendOutput(env, $result, obj)

#define SWIG_Object YogVal
%include <typemaps/swigmacros.swg>
%include <typemaps/fragments.swg>

%fragment(SWIG_AsVal_frag(long), "header") {
SWIGINTERN int
SWIG_AsVal_dec(long)(YogVal obj, long* val)
{
    if (IS_FIXNUM(obj)) {
        if (val != NULL) {
            *val = VAL2INT(obj);
        }
        return SWIG_OK;
    }
    return SWIG_TypeError;
}
}

%fragment(SWIG_From_frag(long), "header") {
    %define_as(SWIG_From_dec(long), INT2VAL)
}

%fragment(SWIG_AsVal_frag(bool),"header") {
SWIGINTERN int
SWIG_AsVal_dec(bool)(YogVal obj, bool* val)
{
    bool b;
    if (IS_TRUE(obj)) {
        b = true;
    }
    else {
        b = false;
    }
    if (val != NULL) {
        *val = b;
    }
    return SWIG_OK;
}
}

%include <typemaps/swigtypemaps.swg>
%runtime "swigrun.swg"
%runtime "swigerrors.swg"

%runtime %{
#include <assert.h>
#include <stdlib.h>
#include "yog/array.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define SWIG_Yog_ConvertPtr(obj, pptr, type, flags) \
    convert_ptr(env, obj, pptr, type, flags, 0)
#define SWIG_ConvertPtr(obj, pptr, type, flags) \
    SWIG_Yog_ConvertPtr(obj, pptr, type, flags)
#define SWIG_NewPointerObj(ptr, type, flags) \
    Shadow_new(env, pkg, ptr, type, flags)

#define SWIG_POINTER_IMPLICIT_CONV  (SWIG_POINTER_DISOWN << 1)

#define SWIG_Error(code, msg)   raise_error(env, code, msg)
#define SWIG_fail

struct Wrapper {
    const char* name;
    YogVal (*f)(YogEnv*, YogVal, YogVal, YogVal, YogVal, YogVal);
};

typedef struct Wrapper Wrapper;

struct Shadow {
    struct YogBasicObj base;
    void* ptr;
    swig_type_info* ty;
    bool own;
};

typedef struct Shadow Shadow;

#define TYPE_SHADOW     ((type_t)Shadow_new)

struct Package {
    struct YogPackage base;
    YogVal cShadow;
};

typedef struct Package Package;

#define TYPE_PRIVATE_PACKAGE    ((type_t)Package_new)

static void
raise_error(YogEnv* env, int_t code, const char* msg)
{
    YogError_raise_TypeError(env, "%d: %s", code, msg);
}

static void
Package_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogPackage_keep_children(env, ptr, keeper, heap);

    Package* pkg = (Package*)ptr;
#define KEEP(member)    YogGC_keep(env, &pkg->member, keeper, heap)
    KEEP(cShadow);
#undef KEEP
}

static YogVal
Package_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = ALLOC_OBJ(env, Package_keep_children, NULL, Package);
    YogObj_init(env, pkg, TYPE_PRIVATE_PACKAGE, FLAG_PKG, env->vm->cPackage);
    PTR_AS(Package, pkg)->cShadow = YUNDEF;

    YogPackage_init(env, pkg, TYPE_PRIVATE_PACKAGE);

    RETURN(env, pkg);
}

SWIGRUNTIME YogVal
SWIG_Yog_GetSwigThis(YogEnv* env, YogVal obj)
{
    SAVE_ARG(env, obj);
    YogVal thiz = YUNDEF;
    PUSH_LOCAL(env, thiz);

    ID name = YogVM_intern(env, env->vm, "this");
    thiz = YogVal_get_attr(env, obj, name);

    RETURN(env, thiz);
}

static YogVal
Shadow_new(YogEnv* env, YogVal pkg, void* ptr, swig_type_info* ty, int flags)
{
    SAVE_ARG(env, pkg);
    YogVal shadow = YUNDEF;
    YogVal klass = YUNDEF;
    YogVal cShadow = YUNDEF;
    PUSH_LOCALS3(env, shadow, klass, cShadow);
    if (ptr == NULL) {
        RETURN(env, YNIL);
    }
    if (!IS_PTR(pkg) || (BASIC_OBJ_TYPE(pkg) != TYPE_PRIVATE_PACKAGE)) {
        YogError_raise_TypeError(env, "invalid package");
    }
    cShadow = PTR_AS(Package, pkg)->cShadow;

    shadow = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Shadow);
    YogBasicObj_init(env, shadow, TYPE_SHADOW, 0, cShadow);
    PTR_AS(Shadow, shadow)->ptr = ptr;
    PTR_AS(Shadow, shadow)->ty = ty;
    PTR_AS(Shadow, shadow)->own = (flags & SWIG_POINTER_OWN) ? true : false;

    RETURN(env, shadow);
}

SWIGRUNTIME int
convert_ptr(YogEnv* env, YogVal thiz, void** ptr, swig_type_info* ty, int flags, bool* own)
{
    SAVE_ARG(env, thiz);
    YogVal shadow = YUNDEF;
    PUSH_LOCAL(env, shadow);
#define OUTPUT_VALUE(ptr, val)    do { \
    if ((ptr) != NULL) { \
        *(ptr) = (val); \
    } \
} while (0)
    if (!IS_PTR(thiz)) {
        OUTPUT_VALUE(ptr, NULL);
        RETURN(env, SWIG_OK);
    }

    ID name = YogVM_intern(env, env->vm, "this");
    shadow = YogVal_get_attr(env, thiz, name);
    if (IS_UNDEF(shadow)) {
        YogError_raise_AttributeError(env, "attribute \"this\" not found");
    }
    if (!IS_PTR(shadow) || (BASIC_OBJ_TYPE(shadow) != TYPE_SHADOW)) {
        YogError_raise_TypeError(env, "invalid shadow object");
    }

    if (flags & SWIG_POINTER_DISOWN) {
        PTR_AS(Shadow, shadow)->own = false;
    }

    void* vptr = PTR_AS(Shadow, shadow)->ptr;
    if (ty == NULL) {
        OUTPUT_VALUE(ptr, vptr);
        RETURN(env, SWIG_OK);
    }
    swig_cast_info* tc = SWIG_TypeCheckStruct(PTR_AS(Shadow, shadow)->ty, ty);
    if (tc == NULL) {
        RETURN(env, SWIG_ERROR);
    }
    int newmemory;
    OUTPUT_VALUE(ptr, SWIG_TypeCast(tc, vptr, &newmemory));
    RETURN(env, SWIG_OK);
#undef OUTPUT_VALUE
}

SWIGINTERN void
SWIG_SetConstant(YogEnv* env, YogVal dict, const char* name, YogVal obj)
{
    SAVE_ARGS2(env, dict, obj);
    YogVal key = YUNDEF;
    PUSH_LOCAL(env, key);

    key = YogString_from_str(env, name);
    YogDict_set(env, dict, key, obj);

    RETURN_VOID(env);
}

SWIGINTERN void
SWIG_SetErrorObj(YogEnv* env, YogVal errtype, YogVal obj)
{
    YogError_raise(env, obj);
}

SWIGINTERN YogVal
SWIG_AppendOutput(YogEnv* env, YogVal result, YogVal obj)
{
    SAVE_ARGS2(env, result, obj);
    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    if (!IS_PTR(result)) {
        RETURN(env, obj);
    }
    if (BASIC_OBJ_TYPE(result) == TYPE_ARRAY) {
        array = YogArray_of_size(env, 2);
        YogArray_push(env, array, result);
        YogArray_push(env, array, obj);
        RETURN(env, array);
    }

    YogArray_push(env, result, obj);
    RETURN(env, result);
}
%}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
