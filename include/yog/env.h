#if !defined(__YOG_ENV_H__)
#define __YOG_ENV_H__

#include "yog/thread.h"
#include "yog/vm.h"

struct YogEnv {
    struct YogVm* vm;
    struct YogThread* thread;
};

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
