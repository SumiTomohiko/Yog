#include <stdlib.h>
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif

void 
YogThread_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogThread* thread = ptr;

    thread->cur_frame = YogVal_keep(env, thread->cur_frame, keeper);
    thread->jmp_val = YogVal_keep(env, thread->jmp_val, keeper);

    YogLocals* locals = thread->locals;
    while (locals != NULL) {
        unsigned int i;
        for (i = 0; i < locals->num_vals; i++) {
            YogVal* vals = locals->vals[i];
            if (vals == NULL) {
                continue;
            }

            unsigned int j;
            for (j = 0; j < locals->size; j++) {
                YogVal* val = &vals[j];
                DEBUG(DPRINTF("val=%p", val));
                *val = YogVal_keep(env, *val, keeper);
            }
        }

        locals = locals->next;
    }
}

void 
YogThread_initialize(YogEnv* env, YogVal thread)
{
    PTR_AS(YogThread, thread)->cur_frame = YNIL;
    PTR_AS(YogThread, thread)->jmp_buf_list = NULL;
    PTR_AS(YogThread, thread)->jmp_val = YUNDEF;
    PTR_AS(YogThread, thread)->locals = NULL;
}

void 
YogThread_finalize(YogEnv* env, YogThread* thread) 
{
}

YogVal 
YogThread_new(YogEnv* env) 
{
    YogVal thread = ALLOC_OBJ(env, YogThread_keep_children, NULL, YogThread);
    YogThread_initialize(env, thread);

    return thread;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
