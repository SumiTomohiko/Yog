#if !defined(__YOG_ENV_H__)
#define __YOG_ENV_H__

#include "yog/vm.h"

struct YogEnv {
    struct YogVM* vm;
    YogVal thread;
};

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
