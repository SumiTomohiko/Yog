#if !defined(__YOG_OBJECT_H__)
#define __YOG_OBJECT_H__

#include "yog/yog.h"

typedef uint_t type_t;

#if WINDOWS && !defined(YOG_CORE)
    /**
     * = Trick for Windows
     *
     * Assume that you define a function named "foo" in an executable. If you
     * export this function for DLLs to an import library, this function will
     * be "__imp__foo"! For example,
     *
     *   $ cat bar.c
     *   __declspec(dllexport) void
     *   foo()
     *   {
     *   }
     * 
     *   int
     *   main(int argc, const char* argv[])
     *   {
     *       return 0;
     *   }
     *   $ gcc -o bar.exe -Wl,--out-implib,bar.exe.a bar.c
     *   Creating library file: bar.exe.a
     *   $ nm bar.exe.a | grep foo
     *   00000000 I __imp__foo
     *   00000000 T _foo
     *
     * So when compiling packages, we must tell compilers that there is a symbol
     * of __imp__foo and use this instead of foo.
     */
#   define CONCAT_TYPE(name)    _imp__##name
#   define DECL_AS_TYPE(name)   extern void* CONCAT_TYPE(name)
#   define TO_TYPE(name)        ((type_t)CONCAT_TYPE(name))
#else
#   define DECL_AS_TYPE(name)
#   define TO_TYPE(name)        ((type_t)name)
#endif

struct YogBasicObj {
    type_t type;
    uint_t id_upper;
    uint_t id_lower;
    flags_t flags;
    YogVal klass;
};

#define TYPE_BASIC_OBJ  ((type_t)YogBasicObj_init)

#define HAS_ATTRS       (1 << 0)
#define FLAG_PKG        (1 << 1)

#define YOGBASICOBJ_HEAD    struct YogBasicObj base
#define BASIC_OBJ(v)        PTR_AS(YogBasicObj, (v))
#define BASIC_OBJ_TYPE(v)   BASIC_OBJ(v)->type

typedef struct YogBasicObj YogBasicObj;

struct YogObj {
    YOGBASICOBJ_HEAD;
    YogVal attrs;
};

DECL_AS_TYPE(YogObj_new);
#define TYPE_OBJ TO_TYPE(YogObj_new)

#define YOGOBJ_HEAD struct YogObj base
#define YOGOBJ(obj) ((struct YogObj*)obj)

typedef struct YogObj YogObj;

typedef YogVal (*Allocator)(struct YogEnv*, YogVal);

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/object.c */
YOG_EXPORT void YogBasicObj_init(YogEnv*, YogVal, type_t, uint_t, YogVal);
YOG_EXPORT void YogBasicObj_keep_children(YogEnv*, void*, ObjectKeeper, void*);
YOG_EXPORT YogVal YogObj_alloc(YogEnv*, YogVal);
YOG_EXPORT void YogObj_class_init(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogObj_get_attr(YogEnv*, YogVal, ID);
YOG_EXPORT void YogObj_init(YogEnv*, YogVal, uint_t, uint_t, YogVal);
YOG_EXPORT void YogObj_keep_children(YogEnv*, void*, ObjectKeeper, void*);
YOG_EXPORT YogVal YogObj_new(YogEnv*, YogVal);
YOG_EXPORT void YogObj_set_attr(YogEnv*, YogVal, const char*, YogVal);
YOG_EXPORT void YogObj_set_attr_id(YogEnv*, YogVal, ID, YogVal);
YOG_EXPORT void YogObject_boot(YogEnv*, YogVal, YogVal);
YOG_EXPORT void YogObject_eval_builtin_script(YogEnv*, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
