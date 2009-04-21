#include <string.h>
#include "yog/env.h"
#include "yog/gc/generational.h"
#include "yog/vm.h"
#include "yog/yog.h"
#if defined(TEST_GENERATIONAL)
#   include <stdio.h>
#   include <stdlib.h>
#   include <CUnit/Basic.h>
#   include <CUnit/CUnit.h>
#endif

#if 0
#   define DEBUG(x) x
#else
#   define DEBUG(x)
#endif

static void 
oldify(YogEnv* env, YogGenerational* gen, void* ptr) 
{
    YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
    header->servive_num = gen->tenure - 1;
}

static void 
oldify_all_callback(YogEnv* env, YogCopyingHeader* header) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    oldify(env, gen, header + 1);
}

void 
YogGenerational_oldify_all(YogEnv* env, YogGenerational* gen) 
{
    YogCopying_iterate_objects(env, &gen->copying, oldify_all_callback);
}

void* 
YogGenerational_copy_young_object(YogEnv* env, void* ptr, ObjectKeeper obj_keeper)
{
    YogGenerational* gen = &env->vm->gc.generational;
    YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
    DEBUG(DPRINTF("alive: %p (%p)", header, ptr));
    if (header->forwarding_addr != NULL) {
        DEBUG(DPRINTF("moved: %p->%p", header, header->forwarding_addr));
        void* to;
        YogCopyingHeader* forwarding_addr = header->forwarding_addr;
        YogCopying* copying = &env->vm->gc.generational.copying;
        if (YogCopying_is_in_inactive_heap(env, copying, forwarding_addr)) {
            to = forwarding_addr + 1;
            DEBUG(DPRINTF("to=%p", to));
        }
        else {
            to = (YogMarkSweepCompactHeader*)forwarding_addr + 1;
            DEBUG(DPRINTF("to=%p", to));
        }
        return to;
    }

    header->servive_num++;
    if (header->servive_num < gen->tenure) {
        return YogCopying_copy(env, &gen->copying, ptr);
    }
    else {
        YogMarkSweepCompact* msc = &gen->msc;
        ChildrenKeeper keeper = header->keeper;
        Finalizer finalizer = header->finalizer;
        size_t size = header->size - sizeof(YogCopyingHeader);
        void* p = YogMarkSweepCompact_alloc(env, msc, keeper, finalizer, size);
        DEBUG(DPRINTF("tenure: %p (%p)->%p (%p)", header, ptr, (YogMarkSweepCompactHeader*)p - 1, p));
        memcpy(p, ptr, size);
        header->forwarding_addr = (YogMarkSweepCompactHeader*)p - 1;
        YogMarkSweepCompact_mark_recursively(env, p, obj_keeper);
        return p;
    }
}

static void* 
major_gc_keep_object(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }

    if (!IS_YOUNG(ptr)) {
        return YogMarkSweepCompact_mark_recursively(env, ptr, major_gc_keep_object);
    }
    else {
        return YogGenerational_copy_young_object(env, ptr, major_gc_keep_object);
    }
}

static void* 
update_pointer(YogEnv* env, void* ptr) 
{
    DEBUG(DPRINTF("updating: %p", ptr));
    if (ptr == NULL) {
        return NULL;
    }

    if (IS_YOUNG(ptr)) {
        YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
        if (!header->updated) {
            header->updated = TRUE;
            ChildrenKeeper keeper = header->keeper;
            if (keeper != NULL) {
                DEBUG(DPRINTF("ptr=%p, keeper=%p", ptr, keeper));
                (*keeper)(env, ptr, update_pointer);
            }
        }
        return ptr;
    }
    else {
        return YogMarkSweepCompact_update_pointer(env, ptr, update_pointer);
    }
}

static void 
initialize_young_updated_callback(YogEnv* env, YogCopyingHeader* header) 
{
    header->updated = FALSE;
}

static void 
initialize_young_updated(YogEnv* env, YogGenerational* generational) 
{
    YogCopying_iterate_objects(env, &generational->copying, initialize_young_updated_callback);
}

void 
YogGenerational_major_gc(YogEnv* env, YogGenerational* generational) 
{
    DEBUG(DPRINTF("major GC"));
    YogMarkSweepCompact* msc = &generational->msc;
    msc->in_gc = TRUE;
    YogMarkSweepCompact_unmark_all(env, msc);

    YogCopying_initialize_gc(env, &generational->copying);
    YogCopying_do_gc(env, &generational->copying, major_gc_keep_object);
    YogMarkSweepCompact_delete_garbage(env, msc);

    initialize_young_updated(env, generational);
    msc->in_gc = FALSE;
}

static void* 
minor_gc_keep_object(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }
    if (!IS_YOUNG(ptr)) {
        return ptr;
    }

    return YogGenerational_copy_young_object(env, ptr, minor_gc_keep_object);
}

void 
YogGenerational_minor_gc(YogEnv* env, YogGenerational* generational) 
{
    DEBUG(DPRINTF("minor GC"));
    YogMarkSweepCompact* msc = &generational->msc;
    msc->in_gc = TRUE;

    YogCopying_initialize_gc(env, &generational->copying);
    YogMarkSweepCompact_iterate_grey_pages(env, msc);
    YogCopying_do_gc(env, &generational->copying, minor_gc_keep_object);

    msc->in_gc = FALSE;
}

void 
YogGenerational_initialize(YogEnv* env, YogGenerational* generational, BOOL stress, size_t young_heap_size, size_t old_chunk_size, size_t old_threshold, unsigned int tenure, void* root, ChildrenKeeper root_keeper) 
{
    generational->err = ERR_GEN_NONE;

    YogCopying* copying = &generational->copying;
    YogCopying_initialize(env, copying, stress, young_heap_size, root, root_keeper);

    YogMarkSweepCompact* msc = &generational->msc;
    YogMarkSweepCompact_initialize(env, msc, old_chunk_size, old_threshold, root, root_keeper);

    generational->tenure = tenure;
}

void 
YogGenerational_finalize(YogEnv* env, YogGenerational* generational) 
{
    generational->err = ERR_GEN_NONE;

    YogMarkSweepCompact* msc = &generational->msc;
    YogMarkSweepCompact_finalize(env, msc);

    YogCopying* copying = &generational->copying;
    YogCopying_finalize(env, copying);
}

void* 
YogGenerational_alloc(YogEnv* env, YogGenerational* generational, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    YogCopying* copying = &generational->copying;
    void* ptr = YogCopying_alloc(env, copying, keeper, finalizer, size);
    if (ptr != NULL) {
        DEBUG(DPRINTF("alloc new: %p (%p)", (YogCopyingHeader*)ptr - 1, ptr));
        return ptr;
    }

    YogMarkSweepCompact* msc = &generational->msc;
    ptr = YogMarkSweepCompact_alloc(env, msc, keeper, finalizer, size);
    if (ptr != NULL) {
        DEBUG(DPRINTF("alloc old: %p (%p)", (YogMarkSweepCompactHeader*)ptr - 1, ptr));
        return ptr;
    }

    unsigned int err;
    switch (msc->err) {
    case ERR_MSC_MMAP:
        err = ERR_GEN_MMAP;
        break;
    case ERR_MSC_MUNMAP: 
        err = ERR_GEN_MUNMAP;
        break;
    case ERR_MSC_MALLOC: 
        err = ERR_GEN_MALLOC;
        break;
    default:
        err = ERR_GEN_UNKNOWN;
        break;
    }
    generational->err = err;

    return NULL;
}

#if defined(TEST_GENERATIONAL)
#define CHUNK_SIZE  (1 * 1024 * 1024)
#define THRESHOLD   CHUNK_SIZE
#define HEAP_SIZE   (1 * 1024 * 1024)
#define TENURE      32

#define CREATE_TEST(name, root, root_keeper) \
    static void \
    name() \
    { \
        YogThread thread; \
        YogVal pthread = PTR2VAL(&thread); \
        YogThread_initialize(NULL, pthread); \
        YogVm vm; \
        vm.thread = &thread; \
        YogEnv env; \
        env.vm = &vm; \
        env.thread = &thread; \
        YogMarkSweepCompact_install_sigsegv_handler(&env); \
        YogGenerational_initialize(&env, &vm.gc.generational, FALSE, HEAP_SIZE, CHUNK_SIZE, THRESHOLD, TENURE, root, root_keeper); \
        \
        test_##name(&env); \
        \
        YogGenerational_finalize(&env, &vm.gc.generational); \
    }

static void 
test_alloc1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    void* ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL(ptr);
}

CREATE_TEST(alloc1, NULL, NULL);

static void 
dummy_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
}

static void 
test_alloc2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    void* ptr = YogGenerational_alloc(env, gen, NULL, NULL, HEAP_SIZE + 1);
    CU_ASSERT_PTR_NOT_NULL(ptr);
}

CREATE_TEST(alloc2, NULL, NULL);

static unsigned char* minor_gc1_ptr = NULL;

static void
minor_gc1_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    minor_gc1_ptr = (*keeper)(env, minor_gc1_ptr);
}

static void 
test_minor_gc1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    minor_gc1_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    unsigned char* minor_gc1_ptr_old = minor_gc1_ptr;

    YogGenerational_minor_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((heap->items <= minor_gc1_ptr) && (minor_gc1_ptr <= heap->items + heap->size));
    CU_ASSERT_PTR_NOT_EQUAL(minor_gc1_ptr, minor_gc1_ptr_old);
}

CREATE_TEST(minor_gc1, NULL, minor_gc1_keep_children);

static unsigned char* minor_gc2_ptr = NULL;

static void
minor_gc2_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    minor_gc2_ptr = (*keeper)(env, minor_gc2_ptr);
}

static void 
test_minor_gc2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    minor_gc2_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    oldify(env, gen, minor_gc2_ptr);

    YogGenerational_minor_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((minor_gc2_ptr < heap->items) || (heap->items + heap->size < minor_gc2_ptr));
}

CREATE_TEST(minor_gc2, NULL, minor_gc2_keep_children);

static unsigned char* major_gc1_ptr = NULL;

static void
major_gc1_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    major_gc1_ptr = (*keeper)(env, major_gc1_ptr);
}

static void 
test_major_gc1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    major_gc1_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    unsigned char* major_gc1_ptr_old = major_gc1_ptr;

    YogGenerational_major_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((heap->items <= major_gc1_ptr) && (major_gc1_ptr <= heap->items + heap->size));
    CU_ASSERT_PTR_NOT_EQUAL(major_gc1_ptr, major_gc1_ptr_old);
}

CREATE_TEST(major_gc1, NULL, major_gc1_keep_children);

static unsigned char* major_gc2_ptr = NULL;

static void
major_gc2_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    major_gc2_ptr = (*keeper)(env, major_gc2_ptr);
}

static void 
test_major_gc2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    major_gc2_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    oldify(env, gen, major_gc2_ptr);

    YogGenerational_major_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((major_gc2_ptr < heap->items) || (heap->items + heap->size < major_gc2_ptr));
}

CREATE_TEST(major_gc2, NULL, major_gc2_keep_children);

static void
major_gc3_ptr_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    *(void**)ptr = (*keeper)(env, *(void**)ptr);
}

static unsigned char* major_gc3_ptr = NULL;

static void
major_gc3_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    major_gc3_ptr = (*keeper)(env, major_gc3_ptr);
}

static void 
test_major_gc3(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    major_gc3_ptr = YogGenerational_alloc(env, gen, major_gc3_ptr_keep_children, NULL, sizeof(void*));
    *(void**)major_gc3_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    unsigned char* major_gc3_ptr_old = *(void**)major_gc3_ptr;

    YogGenerational_major_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((heap->items <= (unsigned char*)(*(void**)major_gc3_ptr)) && ((unsigned char*)(*(void**)major_gc3_ptr) <= heap->items + heap->size));
    CU_ASSERT_PTR_NOT_EQUAL(major_gc3_ptr, major_gc3_ptr_old);
}

CREATE_TEST(major_gc3, NULL, major_gc3_keep_children);

static BOOL finalize1_flag = FALSE;

static void 
finalize1_finalize(YogEnv* env, void* ptr) 
{
    finalize1_flag = TRUE;
}

static void 
test_finalize1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    YogGenerational_alloc(env, gen, NULL, finalize1_finalize, 0);
    YogGenerational_minor_gc(env, gen);
    CU_ASSERT_TRUE(finalize1_flag);
}

CREATE_TEST(finalize1, NULL, dummy_keeper);

static BOOL finalize2_flag = FALSE;
static void* finalize2_ptr = NULL;

static void 
finalize2_finalize(YogEnv* env, void* ptr) 
{
    finalize2_flag = TRUE;
}

static void 
finalize2_root_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    finalize2_ptr = (*keeper)(env, finalize2_ptr);
}

static void 
test_finalize2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    finalize2_ptr = YogGenerational_alloc(env, gen, NULL, finalize2_finalize, 0);
    oldify(env, gen, finalize2_ptr);
    YogGenerational_minor_gc(env, gen);

    finalize2_ptr = NULL;
    YogGenerational_major_gc(env, gen);

    CU_ASSERT_TRUE(finalize2_flag);
}

CREATE_TEST(finalize2, NULL, finalize2_root_keeper);

static void* forwarding_addr1_ptr1 = NULL;
static void* forwarding_addr1_ptr2 = NULL;

static void 
forwarding_addr1_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    forwarding_addr1_ptr1 = (*keeper)(env, forwarding_addr1_ptr1);
    forwarding_addr1_ptr2 = (*keeper)(env, forwarding_addr1_ptr2);
}

static void 
test_forwarding_addr1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    forwarding_addr1_ptr1 = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    forwarding_addr1_ptr2 = forwarding_addr1_ptr1;
    oldify(env, gen, forwarding_addr1_ptr1);
    YogGenerational_major_gc(env, gen);

    CU_ASSERT_PTR_EQUAL(forwarding_addr1_ptr1, forwarding_addr1_ptr2);
}

CREATE_TEST(forwarding_addr1, NULL, forwarding_addr1_keep_children);

static void* forwarding_addr2_ptr1 = NULL;
static void* forwarding_addr2_ptr2 = NULL;

static void 
forwarding_addr2_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    forwarding_addr2_ptr1 = (*keeper)(env, forwarding_addr2_ptr1);
    forwarding_addr2_ptr2 = (*keeper)(env, forwarding_addr2_ptr2);
}

static void 
test_forwarding_addr2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    forwarding_addr2_ptr1 = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    forwarding_addr2_ptr2 = forwarding_addr2_ptr1;
    YogGenerational_minor_gc(env, gen);

    CU_ASSERT_PTR_EQUAL(forwarding_addr2_ptr1, forwarding_addr2_ptr2);
}

CREATE_TEST(forwarding_addr2, NULL, forwarding_addr2_keep_children);

static void* compact1_ptr = NULL;

static void 
compact1_ptr_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    *(void**)ptr = (*keeper)(env, *(void**)ptr);
}

static void 
compact1_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    compact1_ptr = (*keeper)(env, compact1_ptr);
}

static void 
test_compact1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    compact1_ptr = YogGenerational_alloc(env, gen, compact1_ptr_keep_children, NULL, sizeof(void*));
    *(void**)compact1_ptr = compact1_ptr;
    oldify(env, gen, compact1_ptr);
    YogGenerational_minor_gc(env, gen);
    YogGenerational_major_gc(env, gen);
    CU_ASSERT_PTR_EQUAL(compact1_ptr, *(void**)compact1_ptr);
}

CREATE_TEST(compact1, NULL, compact1_keep_children);

static void* compact2_ptr = NULL;

static void 
compact2_ptr_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    *(void**)compact2_ptr = (*keeper)(env, *(void**)compact2_ptr);
}

static void 
compact2_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    compact2_ptr = (*keeper)(env, compact2_ptr);
}

static void 
test_compact2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    compact2_ptr = YogGenerational_alloc(env, gen, compact2_ptr_keep_children, NULL, sizeof(void*));
    *(void**)compact2_ptr = compact2_ptr;
    YogGenerational_major_gc(env, gen);
    CU_ASSERT_PTR_EQUAL(compact2_ptr, *(void**)compact2_ptr);
}

CREATE_TEST(compact2, NULL, compact2_keep_children);

static void* grey_page1_ptr;

static void
grey_page1_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    grey_page1_ptr = (*keeper)(env, grey_page1_ptr);
}

static void 
grey_page1_ptr_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    *(void**)ptr = (*keeper)(env, *(void**)ptr);
}

static void 
test_grey_page1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    grey_page1_ptr = YogGenerational_alloc(env, gen, grey_page1_ptr_keep_children, NULL, sizeof(void*));
    *(void**)grey_page1_ptr = NULL;
    oldify(env, gen, grey_page1_ptr);
    YogGenerational_minor_gc(env, gen);

    YogMarkSweepCompact_grey_page(grey_page1_ptr);

    *(void**)grey_page1_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    void* grey_page1_ptr_old = *(void**)grey_page1_ptr;
    YogGenerational_minor_gc(env, gen);

    CU_ASSERT_PTR_NOT_EQUAL_FATAL(*(void**)grey_page1_ptr, grey_page1_ptr_old);
}

CREATE_TEST(grey_page1, NULL, grey_page1_keep_children);

#define PRIVATE

PRIVATE int 
main(int argc, const char* argv[]) 
{
#define ERROR(...)  do { \
    fprintf(stderr, __VA_ARGS__); \
    exit(-1); \
} while (0)
    if (CU_initialize_registry() != CUE_SUCCESS) {
        ERROR("failed CU_initialize_registry");
    }

    CU_pSuite suite = CU_add_suite("generational", NULL, NULL);
    if (suite == NULL) {
        ERROR("failed CU_add_suite");
    }

#define ADD_TEST(name)  do { \
    CU_pTest test = CU_add_test(suite, #name, name); \
    if (test == NULL) { \
        ERROR("failed CU_add_test %s", #name); \
    } \
} while (0)
    ADD_TEST(alloc1);
    ADD_TEST(alloc2);
    ADD_TEST(compact1);
    ADD_TEST(compact2);
    ADD_TEST(finalize1);
    ADD_TEST(finalize2);
    ADD_TEST(forwarding_addr1);
    ADD_TEST(forwarding_addr2);
    ADD_TEST(grey_page1);
    ADD_TEST(major_gc1);
    ADD_TEST(major_gc2);
    ADD_TEST(major_gc3);
    ADD_TEST(minor_gc1);
    ADD_TEST(minor_gc2);
#undef ADD_TEST

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();

    return 0;
#undef ERROR
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
