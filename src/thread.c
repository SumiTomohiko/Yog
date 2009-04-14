#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif

void 
YogThreadCtx_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogThreadCtx* thread_ctx = ptr;

    thread_ctx->cur_frame = YogVal_keep(env, thread_ctx->cur_frame, keeper);
    thread_ctx->jmp_val = YogVal_keep(env, thread_ctx->jmp_val, keeper);

    YogLocals* locals = thread_ctx->locals;
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

#if defined(GC_GENERATIONAL)
    YogVal** p;
    for (p = thread_ctx->ref_tbl; p < thread_ctx->ref_tbl_ptr; p++) {
        **p = YogVal_keep(env, **p, keeper);
    }
#endif
}

#if defined(GC_GENERATIONAL)
void 
YogThreadCtx_shrink_ref_tbl(YogEnv* env, YogThreadCtx* thread) 
{
    YogVal** to = thread->ref_tbl;
    YogVal** p;
    for (p = thread->ref_tbl; p < thread->ref_tbl_ptr; p++) {
        YogCopying* copying = &env->vm->gc.generational.copying;
        if (YogCopying_is_in_active_heap(env, copying, VAL2PTR(**p))) {
            *to = *p;
            to++;
        }
    }
    thread->ref_tbl_ptr = to;
}
#endif

void 
YogThreadCtx_initialize(YogEnv* env, YogVal thread)
{
    PTR_AS(YogThreadCtx, thread)->cur_frame = YNIL;
    PTR_AS(YogThreadCtx, thread)->jmp_buf_list = NULL;
    PTR_AS(YogThreadCtx, thread)->jmp_val = YUNDEF;
    PTR_AS(YogThreadCtx, thread)->locals = NULL;
#if defined(GC_GENERATIONAL)
#   define REF_TBL_SIZE     256
    PTR_AS(YogThreadCtx, thread)->ref_tbl = malloc(REF_TBL_SIZE * sizeof(YogVal*));
    PTR_AS(YogThreadCtx, thread)->ref_tbl_limit = PTR_AS(YogThreadCtx, thread)->ref_tbl + REF_TBL_SIZE;
    PTR_AS(YogThreadCtx, thread)->ref_tbl_ptr = PTR_AS(YogThreadCtx, thread)->ref_tbl;
#   undef REF_TBL_SIZE
#endif
}

void 
YogThreadCtx_finalize(YogEnv* env, YogThreadCtx* thread) 
{
#if defined(GC_GENERATIONAL)
    free(thread->ref_tbl);
#endif
}

YogVal 
YogThreadCtx_new(YogEnv* env) 
{
    YogVal thread_ctx = ALLOC_OBJ(env, YogThreadCtx_keep_children, NULL, YogThreadCtx);
    YogThreadCtx_initialize(env, thread_ctx);

    return thread_ctx;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
