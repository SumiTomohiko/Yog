#if !defined(YOG_DATETIME_H_INCLUDED)
#define YOG_DATETIME_H_INCLUDED
#include <time.h>
#include "yog/yog.h"

/**
 * TODO: This header is out of update_prototype.py, because it detects
 * IMPLEMENTE_PROPERTY as a function.
 * Issue: bf6823d9105621e752cbde6da7e738f8f34e7c61
 */
YogVal YogDatetime_new(YogEnv*, time_t);
void YogDatetime_define_classes(YogEnv*, YogHandle*);
void YogDatetime_eval_builtin_script(YogEnv*, YogVal);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
