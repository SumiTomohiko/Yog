#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "yog/error.h"
#include "yog/gc.h"
#if defined(GC_COPYING)
#   include "yog/gc/copying.h"
#elif defined(GC_MARK_SWEEP)
#   include "yog/gc/mark-sweep.h"
#elif defined(GC_MARK_SWEEP_COMPACT)
#   include "yog/gc/mark-sweep-compact.h"
#else
#   include "yog/gc/internal.h"
#   include "yog/gc/generational.h"
#endif
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/sysdeps.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define MAIN_THREAD(vm)     (vm)->main_thread

typedef void (*GC)(YogEnv*);

static YogVal*
alloc_marked_objects(YogVal* old, uint_t size)
{
    YogVal* p = (YogVal*)realloc(old, sizeof(YogVal) * size);
    if (p == NULL) {
        fputs("Cannot allocate memory for marked objects.", stderr);
        exit(1);
    }
    return p;
}

void
YogHeap_finalize(YogEnv* env, YogHeap* heap)
{
    uint_t i;
    for (i = 0; i < array_sizeof(heap->marked_objects); i++) {
        free(heap->marked_objects[i].ptr);
    }
}

static void
swap_marked_objects(YogHeap* heap)
{
    YogMarkedObjects* tmp = heap->cur_marked_objects;
    heap->cur_marked_objects = heap->prev_marked_objects;
    heap->prev_marked_objects = tmp;
}

void
YogHeap_prepare_marking(YogEnv* env, YogHeap* heap)
{
    swap_marked_objects(heap);
    heap->cur_marked_objects->pos = 0;
}

static void
extend_marked_objects(YogEnv* env, YogMarkedObjects* mo)
{
    uint_t new_size = mo->size + 1024;
    const char* fmt = "mo->size=%u, new_size=%u";
    YOG_ASSERT(env, mo->size < new_size, fmt, mo->size, new_size);

    mo->ptr = alloc_marked_objects(mo->ptr, new_size);
    mo->size = new_size;
}

void
YogHeap_add_to_marked_objects(YogEnv* env, YogHeap* heap, YogVal v)
{
    if (!IS_PTR(v)) {
        return;
    }
    YogMarkedObjects* mo = heap->cur_marked_objects;
    uint_t pos = mo->pos;
    if (pos == mo->size) {
        extend_marked_objects(env, mo);
    }
    mo->ptr[pos] = v;
    mo->pos++;
}

BOOL
YogHeap_is_marked_objects_empty(YogEnv* env, YogHeap* heap)
{
    return heap->cur_marked_objects->pos == 0;
}

static void
wakeup_gc_thread(YogEnv* env)
{
    DEBUG(TRACE("%p: enter wakeup_gc_thread", env));
    YogVM* vm = env->vm;
    if (pthread_cond_signal(&vm->threads_suspend_cond) != 0) {
        YOG_BUG(env, "pthread_cond_signal failed");
    }
    DEBUG(TRACE("%p: exit wakeup_gc_thread", env));
}

static void
wait_condition_variable(YogEnv* env, pthread_cond_t* cond, pthread_mutex_t* mutex)
{
    if (pthread_cond_wait(cond, mutex) != 0) {
        YOG_BUG(env, "pthread_cond_wait failed");
    }
}

static void
wait_gc_finish(YogEnv* env)
{
    DEBUG(TRACE("%p: enter wait_gc_finish", env));
    YogVM* vm = env->vm;
    uint_t id = vm->gc_id;
    while (vm->running_gc && (vm->gc_id == id)) {
        pthread_cond_t* cond = &vm->gc_finish_cond;
        pthread_mutex_t* mutex = &vm->global_interp_lock;
        wait_condition_variable(env, cond, mutex);
    }
    DEBUG(TRACE("%p: exit wait_gc_finish", env));
}

static void
decrement_suspend_counter(YogEnv* env)
{
    DEBUG(TRACE("%p: enter decrement_suspend_counter", env));
    YogVM* vm = env->vm;
    vm->suspend_counter--;
    if (vm->suspend_counter == 0) {
        wakeup_gc_thread(env);
    }
    DEBUG(TRACE("%p: exit decrement_suspend_counter", env));
}

/**
 * This function assumes that caller holds global interpreter lock.
 */
void
YogGC_suspend(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_suspend", env));
    decrement_suspend_counter(env);
    wait_gc_finish(env);
    DEBUG(TRACE("%p: exit YogGC_suspend", env));
}

YogVal
YogGC_alloc(YogEnv* env, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    DEBUG(TRACE("%p: enter YogGC_alloc: keeper=%p, finalizer=%p, size=%u", env, keeper, finalizer, size));
    YogVM* vm = env->vm;
    if (vm->waiting_suspend) {
        YogHandle_sync_scope_with_env(env);
        YogVM_acquire_global_interp_lock(env, vm);
        YogGC_suspend(env);
        YogVM_release_global_interp_lock(env, vm);
    }

    YogVal thread = env->thread;
#if defined(GC_COPYING)
#   define ALLOC    YogCopying_alloc
#elif defined(GC_MARK_SWEEP)
#   define ALLOC    YogMarkSweep_alloc
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define ALLOC    YogMarkSweepCompact_alloc
#elif defined(GC_GENERATIONAL)
#   define ALLOC    YogGenerational_alloc
#endif
    void* ptr = ALLOC(env, PTR_AS(YogThread, thread)->heap, keeper, finalizer, size);
#undef ALLOC

    DEBUG(TRACE("%p: exit YogGC_alloc", env));
    if (ptr == NULL) {
        YogError_out_of_memory(env, size);
    }

    return PTR2VAL(ptr);
}

static void
wakeup_suspend_threads(YogEnv* env)
{
    DEBUG(TRACE("%p: enter wakeup_suspend_threads", env));
    YogVM* vm = env->vm;
    if (pthread_cond_broadcast(&vm->gc_finish_cond) != 0) {
        YOG_BUG(env, "pthread_cond_broadcast failed");
    }
    DEBUG(TRACE("%p: exit wakeup_suspend_threads", env));
}

static void
wait_suspend(YogEnv* env)
{
    DEBUG(TRACE("%p: enter wait_suspend", env));
    YogVM* vm = env->vm;
    while (vm->suspend_counter != 0) {
        pthread_cond_t* cond = &vm->threads_suspend_cond;
        pthread_mutex_t* mutex = &vm->global_interp_lock;
        wait_condition_variable(env, cond, mutex);
    }
    DEBUG(TRACE("%p: exit wait_suspend", env));
}

static uint_t
count_running_threads(YogEnv* env, YogVM* vm)
{
    DEBUG(TRACE("%p: enter count_running_threads: vm=%p", env, vm));
    uint_t n = 0;
    YogVal thread = vm->running_threads;
    while (IS_PTR(thread)) {
        n += PTR_AS(YogThread, thread)->gc_bound ? 1 : 0;
        thread = PTR_AS(YogThread, thread)->next;
    }

    DEBUG(TRACE("%p: exit count_running_threads: n=%u", env, n));
    return n;
}

static void
run_gc(YogEnv* env, GC gc)
{
    DEBUG(TRACE("%p: enter run_gc: gc=%p", env, gc));
    YogVM* vm = env->vm;
    uint_t threads_num = count_running_threads(env, vm);
    if (0 < threads_num) {
        vm->suspend_counter = threads_num - 1;
        vm->waiting_suspend = TRUE;
        wait_suspend(env);
        (*gc)(env);
        vm->waiting_suspend = FALSE;
    }
    DEBUG(TRACE("%p: exit run_gc", env));
}

static void
perform(YogEnv* env, GC gc)
{
    DEBUG(TRACE("%p: enter perform: gc=%p", env, gc));
    YogHandle_sync_scope_with_env(env);
    YogVM* vm = env->vm;
    YogVM_acquire_global_interp_lock(env, vm);
    if (vm->waiting_suspend) {
        YogGC_suspend(env);
    }
    else {
        vm->running_gc = TRUE;
        run_gc(env, gc);
        vm->running_gc = FALSE;
        wakeup_suspend_threads(env);
        vm->gc_id++;
    }
    YogVM_release_global_interp_lock(env, vm);
    DEBUG(TRACE("%p: exit perform", env));
}

void
YogHeap_init(YogEnv* env, YogHeap* heap)
{
    heap->prev = heap->next = NULL;
    heap->refered = TRUE;

    uint_t i;
    for (i = 0; i < array_sizeof(heap->marked_objects); i++) {
        YogMarkedObjects* marked = &heap->marked_objects[i];
        marked->ptr = NULL;
        marked->size = marked->pos = 0;
    }
    heap->cur_marked_objects = &heap->marked_objects[0];
    heap->prev_marked_objects = &heap->marked_objects[1];
}

void
YogGC_init_memory(YogEnv* env, void* ptr, size_t size)
{
    memset(ptr, 0xcb, size);
}

void*
YogGC_malloc(YogEnv* env, size_t size)
{
    void* ptr = malloc(size);
    if (ptr == NULL) {
        YogError_out_of_memory(env, size);
    }
    YogGC_init_memory(env, ptr, size);
    return ptr;
}

void
YogGC_free(YogEnv* env, void* p, size_t size)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0xfd, size);
    free(p);
}

static void
delete_heap(YogEnv* env, YogHeap* heap)
{
    if (heap->refered) {
        return;
    }

#if defined(GC_COPYING)
#   define IS_EMPTY     YogCopying_is_empty
#   define DELETE       YogCopying_delete
#elif defined(GC_MARK_SWEEP)
#   define IS_EMPTY     YogMarkSweep_is_empty
#   define DELETE       YogMarkSweep_delete
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define IS_EMPTY     YogMarkSweepCompact_is_empty
#   define DELETE       YogMarkSweepCompact_delete
#elif defined(GC_GENERATIONAL)
#   define IS_EMPTY     YogGenerational_is_empty
#   define DELETE       YogGenerational_delete
#endif
    if (!IS_EMPTY(env, heap)) {
        return;
    }

    YogVM* vm = env->vm;
    if (vm->last_heap == heap) {
        vm->last_heap = heap->prev;
    }
    DELETE_FROM_LIST(env->vm->heaps, heap);

    DELETE(env, heap);
#undef DELETE
#undef IS_EMPTY
#undef FINALIZE
}

static void
delete_heaps(YogEnv* env)
{
    YogHeap* heap = env->vm->heaps;
    while (heap != NULL) {
        YogHeap* next = heap->next;
        delete_heap(env, heap);
        heap = next;
    }
}

#define ITERATE_HEAPS(vm, proc)     do { \
    YogHeap* heap; \
    for (heap = (vm)->heaps; heap != NULL; heap = heap->next) { \
        proc; \
    } \
} while (0)

static void
iterate_heaps(YogEnv* env, void (*f)(YogEnv*, YogHeap*))
{
    YogHeap* heap;
    for (heap = env->vm->heaps; heap != NULL; heap = heap->next) {
        f(env, heap);
    }
}

#if defined(GC_COPYING) || defined(GC_MARK_SWEEP) || defined(GC_MARK_SWEEP_COMPACT)
static void
prepare(YogEnv* env)
{
#if defined(GC_COPYING)
#   define PREPARE(env, heap) YogCopying_prepare(env, heap)
#elif defined(GC_MARK_SWEEP)
#   define PREPARE(env, heap) YogMarkSweep_prepare(env, heap)
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define PREPARE(env, heap)
#endif
    ITERATE_HEAPS(env->vm, PREPARE(env, heap));
#undef PREPARE
}

static void
keep_vm(YogEnv* env)
{
    YogHeap* heap = PTR_AS(YogThread, MAIN_THREAD(env->vm))->heap;
#if defined(GC_COPYING)
#   define KEEP YogCopying_keep_root
#elif defined(GC_MARK_SWEEP)
#   define KEEP YogMarkSweep_keep_root
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define KEEP YogMarkSweepCompact_keep_root
#endif
    KEEP(env, env->vm, YogVM_keep_children, heap);
#undef KEEP
}

#if defined(GC_COPYING)
static void
cheney_scan(YogEnv* env)
{
    iterate_heaps(env, YogCopying_cheney_scan);
}
#endif

#if defined(GC_MARK_SWEEP_COMPACT)
static void
mark_in_breadth_first(YogEnv* env)
{
    iterate_heaps(env, YogMarkSweepCompact_mark_in_breadth_first);
}
#endif

static void
delete_garbage(YogEnv* env)
{
#if defined(GC_COPYING)
#   define DELETE   YogCopying_delete_garbage
#elif defined(GC_MARK_SWEEP)
#   define DELETE   YogMarkSweep_delete_garbage
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define DELETE   YogMarkSweepCompact_delete_garbage
#endif
    iterate_heaps(env, DELETE);
#undef DELETE
}

static void
post_gc(YogEnv* env)
{
#if defined(GC_COPYING)
#   define POST(env, heap) YogCopying_post_gc(env, heap)
#elif defined(GC_MARK_SWEEP)
#   define POST(env, heap)
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define POST(env, heap)
#endif
    ITERATE_HEAPS(env->vm, POST(env, heap));
#undef POST
}

static void
gc(YogEnv* env)
{
    prepare(env);

    iterate_heaps(env, YogHeap_prepare_marking);
    keep_vm(env);

#if defined(GC_COPYING)
    cheney_scan(env);
#elif defined(GC_MARK_SWEEP_COMPACT)
    mark_in_breadth_first(env);
#endif

    delete_garbage(env);
    post_gc(env);
    delete_heaps(env);
#if defined(GC_MARK_SWEEP_COMPACT)
#if 0
    do_compaction(env);
#endif
#endif
}

void
YogGC_perform(YogEnv* env)
{
    perform(env, gc);
}

void
YogGC_delete(YogEnv* env)
{
    prepare(env);
    delete_garbage(env);
    post_gc(env);
    delete_heaps(env);
}
#endif

#if defined(GC_MARK_SWEEP_COMPACT) || defined(GC_GENERATIONAL)
#if 0
static void
init_compactors(YogEnv* env, uint_t size, YogCompactor* compactors)
{
    uint_t i;
    for (i = 0; i < size; i++) {
        YogCompactor_init(env, &compactors[i]);
    }
}
#endif

void
YogGC_compact(YogEnv* env)
{
    /* TODO: under construction */
#if 0
    uint_t heaps = count_heaps(env);
    YogCompactor* compactors = (YogCompactor*)YogSysdeps_alloca(sizeof(YogCompactor) * heaps);
    init_compactors(env, heaps, compactors);
#define EACH_HEAP(proc)     do { \
    YogHeap* heap = env->vm->heaps; \
    uint_t i = 0; \
    while (heap != NULL) { \
        proc; \
        heap = heap->next; \
        i++; \
    } \
} while (0)
    EACH_HEAP(YogMarkSweepCompact_alloc_virtually(env, heap, &compactors[i]));
    YogMarkSweepCompactPage** first_free_pages = (YogMarkSweepCompactPage**)YogSysdeps_alloca(sizeof(YogMarkSweepCompactPage*) * heaps);
    EACH_HEAP(first_free_pages[i] = compactors[i].next_page);

    YogVM* vm = env->vm;
    ITERATE_HEAPS(vm, YogMarkSweepCompact_prepare(env, heap));
    YogVM_keep_children(env, vm, YogMarkSweepCompact_update_ptr, PTR_AS(YogThread, MAIN_THREAD(vm))->heap);
    ITERATE_HEAPS(vm, YogMarkSweepCompact_update_front_header(env, heap));

    init_compactors(env, heaps, compactors);
    EACH_HEAP(YogMarkSweepCompact_move_objs(env, heap, &compactors[i]));

    EACH_HEAP(YogMarkSweepCompact_shrink(env, heap, &compactors[i], first_free_pages[i]));
#undef EACH_HEAP
#endif
}
#endif


#if defined(GC_GENERATIONAL)
void
YogGC_add_to_remembered_set(YogEnv* env, void* ptr)
{
    YogHeap* heap = PTR_AS(YogThread, env->thread)->heap;
    YogGenerational_add_to_remembered_set(env, heap, ptr);
}

static void
prepare_minor(YogEnv* env)
{
    iterate_heaps(env, YogGenerational_prepare_minor);
}

static void
prepare_major(YogEnv* env)
{
    iterate_heaps(env, YogGenerational_prepare_major);
}

static void
minor_keep_vm(YogEnv* env)
{
    YogVal main_thread = MAIN_THREAD(env->vm);
    YogGenerational_minor_keep_vm(env, PTR_AS(YogThread, main_thread)->heap);
}

static BOOL
is_finished(YogEnv* env)
{
    BOOL finished = TRUE;
    YogHeap* heap;
    for (heap = env->vm->heaps; (heap != NULL) && finished; heap = heap->next) {
        finished = finished && YogGenerational_is_finished(env, heap);
    }
    return finished;
}

static void
minor_traverse_main(YogEnv* env, YogHeap* heap)
{
    YogGenerational_minor_traverse(env, heap);
    YogHeap_prepare_marking(env, heap);
}

static void
minor_traverse(YogEnv* env)
{
    while (!is_finished(env)) {
        iterate_heaps(env, minor_traverse_main);
    }
}

static void
minor_delete_garbage(YogEnv* env)
{
    iterate_heaps(env, YogGenerational_minor_delete_garbage);
}

static void
minor_post_gc(YogEnv* env)
{
    iterate_heaps(env, YogGenerational_minor_post_gc);
}

static uint_t
count_heaps(YogEnv* env)
{
    uint_t n = 0;
    ITERATE_HEAPS(env->vm, n++);
    return n;
}

static void
minor_gc(YogEnv* env)
{
    DEBUG(TRACE("%p: enter minor_gc", env));

    struct RememberedSetWithHeap {
        struct RememberedSet* remembered_set;
        struct YogHeap* heap;
    };

    typedef struct RememberedSetWithHeap RememberedSetWithHeap;

    uint_t heaps_num = count_heaps(env);
    RememberedSetWithHeap* r = (RememberedSetWithHeap*)YogSysdeps_alloca(sizeof(RememberedSetWithHeap) * heaps_num);
    uint_t i;
    YogHeap* heap;
    for (i = 0, heap = env->vm->heaps; i < heaps_num; i++, heap = heap->next) {
        r[i].remembered_set = YogGenerational_get_remembered_set(env, heap);
        r[i].heap = heap;
    }

    prepare_minor(env);

    iterate_heaps(env, YogHeap_prepare_marking);
    for (i = 0; i < heaps_num; i++) {
        YogHeap* heap = r[i].heap;
        RememberedSet* remembered_set = r[i].remembered_set;
        YogGenerational_trace_remembered_set(env, heap, remembered_set);
    }
    minor_keep_vm(env);

    minor_traverse(env);
    minor_delete_garbage(env);
    minor_post_gc(env);
    delete_heaps(env);

    for (i = 0; i < heaps_num; i++) {
        RememberedSet* remembered_set = r[i].remembered_set;
        uint_t size = SIZEOF_REMEMBERED_SET(remembered_set->size);
        YogGC_free(env, remembered_set, size);
    }

    DEBUG(TRACE("%p: exit minor_gc", env));
}

static void
major_keep_vm(YogEnv* env)
{
    YogVal main_thread = MAIN_THREAD(env->vm);
    YogGenerational_major_keep_vm(env, PTR_AS(YogThread, main_thread)->heap);
}

static void
major_traverse_main(YogEnv* env, YogHeap* heap)
{
    YogGenerational_major_traverse(env, heap);
    YogHeap_prepare_marking(env, heap);
}

static void
major_traverse(YogEnv* env)
{
    while (!is_finished(env)) {
        iterate_heaps(env, major_traverse_main);
    }
}

static void
major_delete_garbage(YogEnv* env)
{
    iterate_heaps(env, YogGenerational_major_delete_garbage);
}

static void
major_post_gc(YogEnv* env)
{
    iterate_heaps(env, YogGenerational_major_post_gc);
}

static void
major_gc(YogEnv* env)
{
    DEBUG(TRACE("%p: enter major_gc", env));
    prepare_major(env);

    iterate_heaps(env, YogHeap_prepare_marking);
    major_keep_vm(env);

    major_traverse(env);
    major_delete_garbage(env);
    major_post_gc(env);
    delete_heaps(env);
    DEBUG(TRACE("%p: exit major_gc", env));
}

void
YogGC_delete(YogEnv* env)
{
    prepare_major(env);
    major_delete_garbage(env);
    major_post_gc(env);
    delete_heaps(env);
}

void
YogGC_perform_minor(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_perform_minor", env));

    perform(env, minor_gc);

    if (env->vm->compaction_flag) {
        YogGC_perform_major(env);
        YogGC_compact(env);
        env->vm->compaction_flag = FALSE;
    }
    else if (env->vm->major_gc_flag) {
        YogGC_perform_major(env);
        env->vm->major_gc_flag = FALSE;
    }

    DEBUG(TRACE("%p: exit YogGC_perform_minor", env));
}

void
YogGC_perform_major(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_perform_major", env));
    perform(env, major_gc);
    DEBUG(TRACE("%p: exit YogGC_perform_major", env));
}
#endif

YogVal
YogGC_keep(YogEnv* env, YogVal val, ObjectKeeper keeper, void* heap)
{
    if (!IS_PTR(val)) {
        return val;
    }
    void* dest = (*keeper)(env, VAL2PTR(val), heap);
    YOG_ASSERT(env, dest != NULL, "Out of memory");
    return PTR2VAL(dest);
}

void
YogGC_free_from_gc(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_free_from_gc", env));
    YogHandle_sync_scope_with_env(env);
    YogVM* vm = env->vm;
    YogVM_acquire_global_interp_lock(env, vm);
    while (vm->waiting_suspend) {
        YogGC_suspend(env);
    }
    PTR_AS(YogThread, env->thread)->gc_bound = FALSE;
    YogVM_release_global_interp_lock(env, vm);
    DEBUG(TRACE("%p: exit YogGC_free_from_gc", env));
}

void
YogGC_bind_to_gc(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_bind_to_gc", env));
    YogVM* vm = env->vm;
    YogVM_acquire_global_interp_lock(env, vm);
    while (vm->waiting_suspend) {
        wait_gc_finish(env);
    }
    PTR_AS(YogThread, env->thread)->gc_bound = TRUE;
    YogVM_release_global_interp_lock(env, vm);
    DEBUG(TRACE("%p: exit YogGC_bind_to_gc", env));
}

void
YogGC_check_multiply_overflow(YogEnv* env, uint_t n, uint_t m)
{
    uint_t l = n * m;
    if ((m == 0) || (l / m == n)) {
        return;
    }
    YOG_BUG(env, "Multiplying %u with %u failed.", n, m);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
