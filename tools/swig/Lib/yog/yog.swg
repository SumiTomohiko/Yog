
%typemap(in) int {
    $1 = VAL2INT($input);
}

%typemap(out) int {
    $result = INT2VAL($1);
}

%init %{
SWIGINTERN swig_module_info*
SWIG_GetModule(void* clientdata)
{
    /* TODO */
    return NULL;
}

SWIGRUNTIME void
SWIG_SetModule(void* clientdata, swig_module_info *swig_module)
{
    /* TODO */
}
%}
%init "swiginit.swg"
%init %{
extern "C" void
SWIG_init(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);

    SWIG_InitializeModule(NULL);

    Wrapper* pf = functions;
    while (pf->name != NULL) {
        YogPackage_define_function(env, pkg, pf->name, (void*)pf->f);
        pf++;
    }

    RETURN_VOID(env);
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
#include "yog/object.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define SWIG_Yog_ConvertPtr(obj, pptr, type, flags) \
    SWIG_Yog_ConvertPtrAndOwn(env, obj, pptr, type, flags, 0)
#define SWIG_ConvertPtr(obj, pptr, type, flags) \
    SWIG_Yog_ConvertPtr(obj, pptr, type, flags)
#define SWIG_NewPointerObj(ptr, type, flags) \
    SWIG_Yog_NewPointerObj(env, ptr, type, flags)
#define SWIG_NewClientData(obj)     SwigYogClientData_New(obj)

#define SWIG_POINTER_IMPLICIT_CONV  (SWIG_POINTER_DISOWN   << 1)

#define SWIG_Error(code, msg)
#define SWIG_fail

struct Wrapper {
    const char* name;
    YogVal (*f)(YogEnv*, YogVal, YogVal, YogVal, YogVal);
};

typedef struct Wrapper Wrapper;

struct SwigYogObject {
    struct YogBasicObj base;
    void* ptr;
    swig_type_info* ty;
    bool own;
    YogVal next;
};

typedef struct SwigYogObject SwigYogObject;

struct SwigYogClientData {
    YogVal klass;
    YogVal newraw;
    YogVal newargs;
    YogVal destroy;
    int delargs;
    int implicitconv;
};

typedef struct SwigYogClientData SwigYogClientData;

SWIGRUNTIME SwigYogClientData*
SwigYogClientData_New(YogVal obj)
{
    if (!IS_PTR(obj)) {
        return 0;
    }
    SwigYogClientData* data = (SwigYogClientData*)malloc(sizeof(SwigYogClientData));
    data->klass = obj;
    return data;
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

SWIGRUNTIME int
SWIG_Yog_ConvertPtrAndOwn(YogEnv* env, YogVal thiz, void** ptr, swig_type_info* ty, int flags, bool* own)
{
    SAVE_ARG(env, thiz);

#define OUTPUT_VALUE(ptr, val)    do { \
    if ((ptr) != NULL) { \
        *(ptr) = (val); \
    } \
} while (0)
    if (!IS_PTR(thiz)) {
        OUTPUT_VALUE(ptr, NULL);
        RETURN(env, SWIG_OK);
    }

    void* vptr = PTR_AS(SwigYogObject, thiz)->ptr;
    OUTPUT_VALUE(own, PTR_AS(SwigYogObject, thiz)->own);

    if (flags & SWIG_POINTER_DISOWN) {
        PTR_AS(SwigYogObject, thiz)->own = false;
    }

    if (ty == NULL) {
        OUTPUT_VALUE(ptr, vptr);
        RETURN(env, SWIG_OK);
    }

#if 0
    if ((ty->clientdata != NULL) && YogVal_is_subclass_of(env, obj, ty->clientdata->klass)) {
        OUTPUT_VALUE(ptr, vptr);
        RETURN(SWIG_OK);
    }
#else
    OUTPUT_VALUE(ptr, vptr);
    RETURN(env, SWIG_OK);
#endif

    const char* c = PTR_AS(SwigYogObject, thiz)->ty->name;
    swig_cast_info* tc = SWIG_TypeCheck(c, ty);
    if (tc == NULL) {
        RETURN(env, SWIG_ERROR);
    }

    int newmemory = 0;
    OUTPUT_VALUE(ptr, SWIG_TypeCast(tc, vptr, &newmemory));
    assert(newmemory != 0);

    RETURN(env, SWIG_OK);
#undef OUTPUT_VALUE
}

static void
SwigYogObject_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    SwigYogObject* obj = (SwigYogObject*)ptr;
#define KEEP(member)    YogGC_keep(env, &obj->member, keeper, heap)
    KEEP(next);
#undef KEEP
}

SWIGRUNTIME YogVal
SwigYogObject_new(YogEnv* env, void* ptr, swig_type_info* ty, bool own)
{
    SAVE_LOCALS(env);
    YogVal sobj = YUNDEF;
    PUSH_LOCAL(env, sobj);

    sobj = ALLOC_OBJ(env, SwigYogObject_keep_children, NULL, SwigYogObject);
    YogBasicObj_init(env, sobj, 0, YUNDEF);
    PTR_AS(SwigYogObject, sobj)->ptr = ptr;
    PTR_AS(SwigYogObject, sobj)->ty = ty;
    PTR_AS(SwigYogObject, sobj)->own = own;
    PTR_AS(SwigYogObject, sobj)->next = YUNDEF;

    RETURN(env, sobj);
}

SWIGRUNTIME YogVal
SWIG_Yog_NewPointerObj(YogEnv* env, void* ptr, swig_type_info* type, int flags)
{
    SAVE_LOCALS(env);
    YogVal robj = YUNDEF;
    PUSH_LOCAL(env, robj);

    if (!ptr) {
        RETURN(env, YUNDEF);
    }
    bool own = (flags & SWIG_POINTER_OWN) ? true : false;
    robj = SwigYogObject_new(env, ptr, type, own);
    RETURN(env, robj);
}

SWIGINTERN void
SWIG_SetConstant(YogEnv* env, YogVal dict, const char* name, YogVal obj)
{
    SAVE_ARGS2(env, dict, obj);
    YogVal key = YUNDEF;
    PUSH_LOCAL(env, key);

    key = YogString_new_str(env, name);
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
    if (IS_OBJ_OF(env, result, cArray)) {
        array = YogArray_new(env);
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
