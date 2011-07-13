#if !defined(YOG_STAT_H_INCLUDED)
#define YOG_STAT_H_INCLUDED
#include "yog/yog.h"

/**
 * TODO: This header is out of update_prototype.py, because it detects
 * IMPLEMENTE_PROPERTY as a function.
 * Issue: bf6823d9105621e752cbde6da7e738f8f34e7c61
 */
YOG_EXPORT YogVal YogStat_lstat(YogEnv*, YogHandle*);
YOG_EXPORT YogVal YogStat_stat(YogEnv*, YogHandle*);
YOG_EXPORT void YogStat_define_classes(YogEnv*, YogHandle*);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
