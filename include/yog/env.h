#if !defined(__YOG_ENV_H__)
#define __YOG_ENV_H__

struct YogEnv {
    struct YogVm* vm;
    struct YogThread* th;
};

#if 0
#define ENV_VM(env)     ((env)->vm)
#define ENV_TH(env)     ((env)->th)
#endif

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
