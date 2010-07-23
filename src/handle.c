#include "yog/config.h"
#if defined(HAVE_STDINT_H)
#   include <stdint.h>
#endif
#if defined(HAVE_STDLIB_H)
#   include <stdlib.h>
#endif
#if defined(HAVE_STRINGS_H)
#   include <strings.h>
#endif
#include "yog/error.h"
#include "yog/private.h"
#include "yog/thread.h"
#include "yog/yog.h"

void
YogHandle_sync_scope_with_env(YogEnv* env)
{
    YogHandleScope* scope = env->handles->scope;
    if (scope == NULL) {
        return;
    }
    scope->pos = env->pos;
    scope->last = env->last;
}

void
YogHandles_init(YogHandles* self)
{
    self->ptr = NULL;
    self->num = self->alloc_num = self->used_num = 0;
    self->scope = NULL;
}

void
YogHandles_finalize(YogHandles* self)
{
    uint_t i;
    for (i = 0;i < self->alloc_num; i++) {
        free(self->ptr[i]);
    }
    free(self->ptr);
}

static YogHandleScope*
get_handle_scope(YogEnv* env)
{
    YogHandleScope* scope = YogThread_get_handle_scope(env, env->thread);
    if (scope != NULL) {
        return scope;
    }
    scope = (YogHandleScope*)malloc(sizeof(YogHandleScope));
    YOG_ASSERT(NULL, scope != NULL, "Can't malloc");
    return scope;
}

static void
YogHandles_extend_handles(YogEnv* env, YogHandles* self)
{
    uint_t num = self->num + 1024;
    size_t size = sizeof(YogHandle*) * self->num;
    YogHandle** ptr = (YogHandle**)realloc(self->ptr, size);
    YOG_ASSERT(env, ptr != NULL, "Can't realloc");
    self->ptr = ptr;
    self->num = num;
}

static void
YogHandles_alloc_handles(YogEnv* env, YogHandles* self)
{
    uint_t pos = self->alloc_num;
    if (self->num <= pos) {
        YogHandles_extend_handles(env, self);
    }
    YogHandle* ptr = (YogHandle*)malloc(HANDLES_SIZE * sizeof(YogHandle));
    YOG_ASSERT(env, ptr != NULL, "Can't malloc");
    self->ptr[pos] = ptr;
    self->alloc_num++;
}

static uint_t
YogHandles_get_handles(YogEnv* env, YogHandles* self)
{
    uint_t pos = self->used_num;
    if (self->alloc_num <= pos) {
        YogHandles_alloc_handles(env, self);
    }
    self->used_num++;
    return pos;
}

static void
YogHandleScope_set_pos(YogEnv* env, YogHandleScope* self, YogHandles* handles)
{
    YogHandle* pos = handles->ptr[handles->used_num - 1];
    env->pos = self->pos = pos;
    env->last = self->last = pos + HANDLES_SIZE;
}

YogHandleScope*
YogHandleScope_open(YogEnv* env)
{
    YogHandle_sync_scope_with_env(env);

    YogHandleScope* next = get_handle_scope(env);
    YogHandles* handles = env->handles;
    uint_t begin = YogHandles_get_handles(env, handles);
    next->begin = begin;
    YogHandleScope_set_pos(env, next, handles);

    next->next = handles->scope;
    handles->scope = next;

    return next;
}

void
YogHandleScope_close(YogEnv* env)
{
    YogHandles* handles = env->handles;
    YogHandleScope* scope = handles->scope;
    YogHandleScope* next = scope->next;
    if (next != NULL) {
        env->pos = next->pos;
        env->last = next->last;
    }

    handles->scope = next;
    handles->used_num = scope->begin;
    if (!YogThread_put_handle_scope(env, env->thread, scope)) {
        free(scope);
    }
}

YogHandle*
YogHandle_register(YogEnv* env, YogVal val)
{
    YogHandle* pos = env->pos;
    if (env->last == pos) {
        YogHandles* handles = env->handles;
        YogHandles_get_handles(env, handles);
        YogHandleScope* scope = handles->scope;
        YogHandleScope_set_pos(env, scope, handles);
        pos = scope->pos;
    }
    pos->val = val;
    env->pos = pos + 1;
    return pos;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
