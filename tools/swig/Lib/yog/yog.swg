
%insert(runtime) %{
#include "yog/array.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/yog.h"

#define SWIG_Yog_ConvertPtr(obj, pptr, type, flags) \
    SWIG_Yog_ConvertPtrAndOwn(env, obj, pptr, type, flags, 0)
#define SWIG_ConvertPtr(obj,pptr,type,flags,own) \
    SWIG_Yog_ConvertPtr(obj, pptr, type, flags, own)
#define SWIG_NewPointerObj(ptr, type, flags) \
    SWIG_Yog_NewPointerObj(env, ptr, type, flags)
#define SWIG_NewClientData(obj)     SwigYogClientData_New(obj)

struct Wrapper {
    const char* name;
    void* f;
};

typedef struct Wrapper Wrapper;

struct SwigYogObject {
    struct YogBasicObj base;
    void* ptr;
    swig_type_info* ty;
    int own;
    YogVal next;
};

typedef struct SwigYogObject SwigYogObject;

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
SWIG_Yog_ConvertPtrAndOwn(YogEnv* env, YogVal obj, void** ptr, swig_type_info* ty, int flags, int* own)
{
    SAVE_ARG(env, obj);
    YogVal thiz = YUNDEF;
    YogVal klass = YUNDEF;
    YogVal impconv = YUNDEF;
    YogVal iobj = YUNDEF;
    PUSH_LOCALS4(env, thiz, klass, impconv, iobj);

#define ASSIGN_PTR(ptr, val)    do { \
    if ((ptr) != NULL) { \
        *(ptr) = (val); \
    } \
} while (0)
    if (!IS_PTR(obj)) {
        RETURN(env, SWIG_ERROR);
    }
    if (IS_NIL(obj)) {
        ASSIGN_PTR(ptr, NULL);
        RETURN(env, SWIG_OK);
    }

    thiz = SWIG_Yog_GetSwigThis(env, obj);
    ASSIGN_PTR(own, 0);
    while (IS_PTR(thiz)) {
        void* vptr = PTR_AS(SwigYogObject, thiz)->ptr;
        if (ty == NULL) {
            ASSIGN_PTR(ptr, vptr);
            break;
        }
        swig_type_info* to = PTR_AS(SwigYogObject, thiz)->ty;
        if (to == ty) {
            ASSIGN_PTR(ptr, vptr);
            break;
        }
        swig_cast_info* tc = SWIG_TypeCheck(to->name, ty);
        if (tc == NULL) {
            if (ptr != NULL) {
                int newmemory = 0;
                *ptr = SWIG_TypeCast(tc, vptr, &newmemory);
                if (newmemory == SWIG_CAST_NEW_MEMORY) {
                    assert(own);
                    ASSIGN_PTR(own, *own | SWIG_CAST_NEW_MEMORY);
                }
            }
            break;
        }
        thiz = PTR_AS(SwigYogObject, thiz)->next;
    }
    if (IS_PTR(thiz)) {
        ASSIGN_PTR(own, *own | PTR_AS(SwigYogObject, thiz)->own);
        if (flags & SWIG_POINTER_DISOWN) {
            PTR_AS(SwigYogObject, thiz)->own = 0;
        }
        RETURN(env, SWIG_OK);
    }
    if (!(flags & SWIG_POINTER_IMPLICIT_CONV)) {
        RETURN(env, SWIG_ERROR);
    }
    SwigYogClientData* data = ty != NULL ? (SwigYogClientData*)ty->clientdata : NULL;
    if ((data == NULL) || data->implicitconv) {
        RETURN(env, SWIG_ERROR);
    }
    klass = data->klass;
    if (!IS_PTR(klass)) {
        RETURN(env, SWIG_ERROR);
    }
    data->implicitconv = 1;
    impconv = SWIG_Yog_CallFunctor(klass, obj);
    data->implicitconv = 0;
    if (!IS_PTR(impconv)) {
        RETURN(env, SWIG_ERROR);
    }
    iobj = SWIG_Yog_GetSwigThis(env, impconv);
    if (!IS_PTR(iobj)) {
        RETURN(env, SWIG_ERROR);
    }
    void* vptr;
    int res = SWIG_Yog_ConvertPtrAndOwn(iobj, &vptr, ty, 0, 0);
    if (!SWIG_IsOK(res)) {
        RETURN(env, SWIG_ERROR);
    }
    if (ptr != NULL) {
        *ptr = vptr;
        PTR_AS(SwigYogObject, iobj)->own = 0;
        res = SWIG_AddCast(SWIG_ERROR);
        res = SWIG_AddNewMask(res);
    }
    else {
        res = SWIG_AddCast(SWIG_ERROR);
    }
    RETURN(env, res);
#undef ASSIGN_PTR
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
SwigYogObject_new(YogEnv* env, void* ptr, swig_type_info* ty, int own)
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

struct SwigYogClientData {
    YogVal klass;
    YogVal newraw;
    YogVal newargs;
    YogVal destroy;
    int delargs;
    int implicitconv;
};

typedef struct SwigYogClientData SwigYogClientData;

SWIGRUNTIME YogVal
SWIG_Yog_NewPointerObj(YogEnv* env, void* ptr, swig_type_info* type, int flags)
{
    SAVE_LOCALS(env);
    YogVal robj = YUNDEF;
    PUSH_LOCAL(env, robj);

    if (!ptr) {
        RETURN(env, YUNDEF);
    }
    int own = (flags & SWIG_POINTER_OWN) ? SWIG_POINTER_OWN : 0;
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
    YogDict_add(env, dict, key, obj);

    RETURN_VOID(env);
}

SWIGINTERN void
SWIG_SetErrorObj(YogEnv* env, YogVal errtype, YogVal obj)
{
    YogError_raise(env, obj);
}

SWIGINTERN PyObject*
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

%typemap(in) int {
    $1 = VAL2INT($input);
}

%typemap(out) int {
    $result = INT2VAL($1);
}

%init %{
extern "C" void
SWIG_init(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);

    Wrapper* pf = functions;
    while (pf->name != NULL) {
        YogPackage_define_function(env, pkg, pf->name, pf->f);
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
    %define_as(SWIG_From_dec(long), YogVal_from_int)
}

%include <typemaps/swigtypemaps.swg>
%runtime "swigrun.swg"

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
