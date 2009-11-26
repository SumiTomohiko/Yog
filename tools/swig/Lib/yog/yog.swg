
%insert(runtime) %{
#include "yog/array.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/yog.h"

#define SWIG_NewPointerObj(ptr, type, flags) \
                                SWIG_Yog_NewPointerObj(env, ptr, type, flags)

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
SWIG_Yog_NewShadowInstance(YogEnv* env, SwigYogClientData* data, YogVal swig_this)
{
#if (PY_VERSION_HEX >= 0x02020000)
  PyObject *inst = 0;
  PyObject *newraw = data->newraw;
  if (newraw) {
    inst = PyObject_Call(newraw, data->newargs, NULL);
    if (inst) {
#if !defined(SWIG_PYTHON_SLOW_GETSET_THIS)
      PyObject **dictptr = _PyObject_GetDictPtr(inst);
      if (dictptr != NULL) {
	PyObject *dict = *dictptr;
	if (dict == NULL) {
	  dict = PyDict_New();
	  *dictptr = dict;
	  PyDict_SetItem(dict, SWIG_This(), swig_this);
	}
      }
#else
      PyObject *key = SWIG_This();
      PyObject_SetAttr(inst, key, swig_this);
#endif
    }
  } else {
#if PY_VERSION_HEX >= 0x03000000
    inst = PyBaseObject_Type.tp_new((PyTypeObject*) data->newargs, Py_None, Py_None);
    PyObject_SetAttr(inst, SWIG_This(), swig_this);
    Py_TYPE(inst)->tp_flags &= ~Py_TPFLAGS_VALID_VERSION_TAG;
#else
    PyObject *dict = PyDict_New();
    PyDict_SetItem(dict, SWIG_This(), swig_this);
    inst = PyInstance_NewRaw(data->newargs, dict);
    Py_DECREF(dict);
#endif
  }
  return inst;
#else
#if (PY_VERSION_HEX >= 0x02010000)
  PyObject *inst;
  PyObject *dict = PyDict_New();
  PyDict_SetItem(dict, SWIG_This(), swig_this);
  inst = PyInstance_NewRaw(data->newargs, dict);
  Py_DECREF(dict);
  return (PyObject *) inst;
#else
  PyInstanceObject *inst = PyObject_NEW(PyInstanceObject, &PyInstance_Type);
  if (inst == NULL) {
    return NULL;
  }
  inst->in_class = (PyClassObject *)data->newargs;
  Py_INCREF(inst->in_class);
  inst->in_dict = PyDict_New();
  if (inst->in_dict == NULL) {
    Py_DECREF(inst);
    return NULL;
  }
#ifdef Py_TPFLAGS_HAVE_WEAKREFS
  inst->in_weakreflist = NULL;
#endif
#ifdef Py_TPFLAGS_GC
  PyObject_GC_Init(inst);
#endif
  PyDict_SetItem(inst->in_dict, SWIG_This(), swig_this);
  return (PyObject *) inst;
#endif
#endif
}

SWIGRUNTIME YogVal
SWIG_Yog_NewPointerObj(YogEnv* env, void* ptr, swig_type_info* type, int flags)
{
    SAVE_LOCALS(env);
    YogVal robj = YUNDEF;
    YogVal inst = YUNDEF;
    PUSH_LOCALS2(env, robj, inst);

    if (!ptr) {
        RETURN(env, YUNDEF);
    }
    int own = (flags & SWIG_POINTER_OWN) ? SWIG_POINTER_OWN : 0;
    robj = SwigYogObject_new(env, ptr, type, own);
    SwigYogClientData* clientdata = type ? (SwigYogClientData*)(type->clientdata) : 0;
    if (!clientdata || (flags & SWIG_POINTER_NOSHADOW)) {
        RETURN(env, robj);
    }
    inst = SWIG_Yog_NewShadowInstance(env, clientdata, robj);
    if (!IS_PTR(inst)) {
        robj = inst;
    }
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
void
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
%include <typemaps/swigtypemaps.swg>

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
