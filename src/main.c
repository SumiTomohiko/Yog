#include "yog/config.h"
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#if defined(__MINGW32__) || defined(_MSC_VER)
#   include <pthread.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(HAVE_WINDOWS_H)
#   include <windows.h>
#endif
#if defined(GC_BDW)
#   include "gc.h"
#endif
#include "getopt.h"
#include "yog/array.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/handle.h"
#include "yog/package.h"
#include "yog/repl.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static void
print_version()
{
#if defined(GC_COPYING)
#   define GC_NAME  "copying"
#elif defined(GC_MARK_SWEEP)
#   define GC_NAME  "mark-sweep"
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define GC_NAME  "mark-sweep-compact"
#elif defined(GC_GENERATIONAL)
#   define GC_NAME  "generational"
#elif defined(GC_BDW)
#   define GC_NAME  "BDW"
#endif
    printf("yog %s %s GC\n", PACKAGE_VERSION, GC_NAME);
#undef GC_NAME
}

static void
usage()
{
    puts("yog [options] [file]");
    puts("options:");
    puts("  --debug-import: print importing log");
    puts("  --gc-stress:");
    puts("  --help: show this message");
    puts("  --heap-size=size:");
    puts("  --version: print version");
}

static size_t
parse_size(const char* s)
{
    size_t total_size = 0;

    const char* c = NULL;
    size_t size = 0;
    for (c = s; *c != '\0'; c++) {
        char ch = tolower(*c);
        if (ch == 'k') {
            total_size += 1024 * size;
            size = 0;
        }
        else if (ch == 'm') {
            total_size += 1024 * 1024 * size;
            size = 0;
        }
        else if (ch == 'g') {
            total_size += 1024 * 1024 * 1024 * size;
            size = 0;
        }
        else {
            if ((ch < '0') || ('9' < ch)) {
                fprintf(stderr, "Invalid size.\n");
                usage();
                exit(1);
            }
            size += 10 * size + (ch - '0');
        }
    }
    total_size += size;

    return total_size;
}

static void
yog_main(YogEnv* env, int_t argc, char* argv[])
{
    if (argc == 0) {
        YogRepl_do(env);
        return;
    }

    const char* filename = argv[0];
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        const char* errmsg = strerror(errno);
        fprintf(stderr, "can't open file \"%s\": %s\n", filename, errmsg);
        return;
    }
    YogEval_eval_file(env, fp, filename, MAIN_MODULE_NAME);
    fclose(fp);
}

static void
enable_gc_stress(YogVM* vm, uint_t gc_stress_level, uint_t level)
{
    if (gc_stress_level < level) {
        return;
    }
    YogVM_enable_gc_stress(NULL, vm);
}

int_t
main(int_t argc, char* argv[])
{
    int_t debug_import = 0;
    uint_t gc_stress_level = 0;
    int_t help = 0;
    size_t young_heap_size = 1 * 1024 * 1024;
    size_t old_heap_size = 1 * 1024 * 1024;
    size_t heap_size = young_heap_size + old_heap_size;
    uint_t max_age = 32;
    struct option options[] = {
        { "debug-import", no_argument, &debug_import, 1 },
        { "gc-stress", no_argument, NULL, 'g' },
        { "heap-size", required_argument, NULL, 'i' },
        { "help", no_argument, &help, 1 },
        { "max-age", required_argument, NULL, 'a' },
        { "old-heap-size", required_argument, NULL, 'o' },
        { "version", no_argument, NULL, 'v' },
        { "young-heap-size", required_argument, NULL, 'y' },
        { NULL, 0, NULL, 0 },
    };
    char c = 0;
    while ((c = getopt_long(argc, argv, "", options, NULL)) != -1) {
        switch (c) {
        case 0:
            break;
        case 'a':
            max_age = atoi(optarg);
            break;
        case 'g':
            gc_stress_level++;
            break;
        case 'i':
            heap_size = parse_size(optarg);
            break;
        case 'o':
            old_heap_size = parse_size(optarg);
            break;
        case 'v':
            print_version();
            exit(0);
            break;
        case 'y':
            young_heap_size = parse_size(optarg);
            break;
        default:
            usage();
            return -1;
            break;
        }
    }

    if (help) {
        usage();
        return 0;
    }

#if defined(__MINGW32__) || defined(_MSC_VER)
    if (!pthread_win32_process_attach_np()) {
        YOG_BUG(NULL, "pthread_win32_process_attach_np failed");
    }
#endif

    YogLocalsAnchor locals = LOCALS_ANCHOR_INIT;
    YogEnv env = ENV_INIT;
    env.locals = &locals;
    YogHandles handles;
    YogHandles_init(&handles);
    env.handles = &handles;

    YogVM vm;
    YogVM_init(&vm);
    enable_gc_stress(&vm, gc_stress_level, 2);
    vm.debug_import = debug_import != 0 ? TRUE : FALSE;
    env.vm = &vm;
    YogVM_add_locals(&env, env.vm, &locals);
    YogVM_add_handles(&env, env.vm, &handles);

    YogThread dummy_thread_body;
    YogVal dummy_thread = PTR2VAL(&dummy_thread_body);
    env.thread = dummy_thread;
    PTR_AS(YogThread, dummy_thread)->pthread = pthread_self();
    YogThread_init(&env, dummy_thread, YUNDEF);
#if defined(GC_BDW)
    GC_INIT();
    YogThread_config_bdw(&env, dummy_thread);
#elif defined(GC_COPYING)
    YogThread_config_copying(&env, dummy_thread, heap_size);
#elif defined(GC_MARK_SWEEP)
    YogThread_config_mark_sweep(&env, dummy_thread, heap_size);
#elif defined(GC_MARK_SWEEP_COMPACT)
    YogThread_config_mark_sweep_compact(&env, dummy_thread, heap_size);
#elif defined(GC_GENERATIONAL)
    YogThread_config_generational(&env, dummy_thread, young_heap_size, old_heap_size, max_age);
#endif
    env.thread = dummy_thread;
    YogVal main_thread = YogThread_new(&env);
    memcpy(VAL2PTR(main_thread), VAL2PTR(dummy_thread), sizeof(YogThread));
    env.thread = main_thread;
    handles.heap = locals.heap = PTR_AS(YogThread, main_thread)->heap;
    YogVM_set_main_thread(&env, &vm, main_thread);

    DECL_LOCALS(env_guard);
    env_guard.num_vals = 3;
    env_guard.size = 1;
    env_guard.vals[0] = &env.thread;
    env_guard.vals[1] = &env.frame;
    env_guard.vals[2] = &main_thread;
    env_guard.vals[3] = NULL;
    PUSH_LOCAL_TABLE(&env, env_guard);

    YogJmpBuf jmpbuf;
    int_t status = setjmp(jmpbuf.buf);
    if (status == 0) {
        YogHandleScope scope;
        YogHandleScope_OPEN(&env, &scope);
        INIT_JMPBUF(&env, jmpbuf);
        PUSH_JMPBUF(env.thread, jmpbuf);

        uint_t yog_argc = argc - optind;
        char** yog_argv = &argv[optind];
        YogVM_boot(&env, env.vm, yog_argc, yog_argv);
        YogVM_configure_search_path(&env, env.vm, argv[0]);

        enable_gc_stress(&vm, gc_stress_level, 1);
        yog_main(&env, yog_argc, yog_argv);

        POP_JMPBUF(&env);
        YogHandleScope_close(&env);
    }
    else {
        YogError_print_stacktrace(&env);
    }

    YogVM_remove_thread(&env, env.vm, env.thread);

    YogVM_wait_finish(&env, env.vm);
    YogVM_remove_handles(&env, env.vm, &handles);
    YogVM_remove_locals(&env, env.vm, &locals);
    YogVM_delete(&env, env.vm);
    YogHandles_finalize(&handles);

#if defined(__MINGW32__) || defined(_MSC_VER)
    pthread_win32_process_detach_np();
#endif

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
