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

static void
YogHandles_extend_handles(YogEnv* env, YogHandles* self)
{
    uint_t num = self->num + 1024;
    size_t size = sizeof(YogHandle*) * num;
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

uint_t
YogHandles_get_handles(YogEnv* env, YogHandles* self)
{
    uint_t pos = self->used_num;
    if (self->alloc_num <= pos) {
        YogHandles_alloc_handles(env, self);
    }
    self->used_num++;
    YOG_ASSERT(env, self->scope != NULL, "No scopes");
    self->scope->used_num++;
    return pos;
}

void
YogHandles_set_pos(YogEnv* env, YogHandles* handles)
{
    YogHandle* pos = handles->ptr[handles->used_num - 1];
    env->pos = pos;
    env->last = pos + HANDLES_SIZE;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
