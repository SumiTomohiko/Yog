#if !defined(__YOG_FILE_H__)
#define __YOG_FILE_H__

#include <stdio.h>
#include "yog/object.h"
#include "yog/yog.h"

struct YogFile {
    struct YogBasicObj base;
    FILE* fp;
};

typedef struct YogFile YogFile;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
#if defined(__cplusplus)
extern "C" {
#endif
/* src/file.c */
YogVal YogFile_define_class(YogEnv*);


#if defined(__cplusplus)
}
#endif
/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
