#if !defined(__YOG_HANDLE_H__)
#define __YOG_HANDLE_H__

#include "yog/yog.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/handle.c */
YOG_EXPORT void YogHandle_sync_scope_with_env(YogEnv*);
YOG_EXPORT void YogHandles_finalize(YogHandles*);
YOG_EXPORT uint_t YogHandles_get_handles(YogEnv*, YogHandles*);
YOG_EXPORT void YogHandles_init(YogHandles*);
YOG_EXPORT void YogHandles_set_pos(YogEnv*, YogHandles*);

/* PROTOTYPE_END */

static inline YogHandle*
YogHandle_register(YogEnv* env, YogVal val)
{
    YogHandle* pos = env->pos;
    if (env->last == pos) {
        YogHandles* handles = env->handles;
        YogHandles_get_handles(env, handles);
        YogHandles_set_pos(env, handles);
        pos = env->pos;
    }
    pos->val = val;
    env->pos = pos + 1;
    return pos;
}

static inline void
YogHandleScope_open(YogEnv* env, YogHandleScope* self, const char* filename, uint_t lineno)
{
    YogHandle_sync_scope_with_env(env);

    YogHandles* handles = env->handles;
    self->used_num = 0;
    env->pos = env->last = NULL;

    self->filename = filename;
    self->lineno = lineno;

    self->next = handles->scope;
    handles->scope = self;
}

#define YogHandleScope_OPEN(env, scope) \
    YogHandleScope_open((env), (scope), __FILE__, __LINE__)

static inline void
YogHandleScope_close(YogEnv* env)
{
    YogHandles* handles = env->handles;
    YogHandleScope* scope = handles->scope;
    YogHandleScope* next = scope->next;
    if (next != NULL) {
        env->pos = next->pos;
        env->last = next->last;
    }
    else {
        env->pos = env->last = NULL;
    }

    handles->scope = next;
    handles->used_num -= scope->used_num;
}

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
