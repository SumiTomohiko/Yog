#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(HAVE_CONFIG_H)
#   include "config.h"
#endif
#include "yog/code.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/package.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"
#if defined(GC_BDW)
#   include "gc.h"
#endif

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
    printf("yog version %s %s GC\n", PACKAGE_VERSION, GC_NAME);
#undef GC_NAME
}

static void 
usage()
{
    puts("yog [options] [file]");
    puts("options:");
    puts("  --debug-parser: ");
    puts("  --gc-stress: ");
    puts("  --help: ");
    puts("  --init-heap-size=size: ");
    puts("  --print-gc-stat: ");
    puts("  --threshold=size: ");
    puts("  --version");
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

int 
main(int argc, char* argv[]) 
{
    int debug_parser = 0;
    int gc_stress = 0;
    int help = 0;
#define DEFAULT_INIT_HEAP_SIZE  (1 * 1024 * 1024)
    size_t init_heap_size = DEFAULT_INIT_HEAP_SIZE;
#undef DEFAULT_INIT_HEAP_SIZE
#define DEFAULT_THRESHOLD   (1 * 1024 * 1024)
    size_t threshold = DEFAULT_THRESHOLD;
#undef DEFAULT_THRESHOLD
    struct option options[] = {
        { "debug-parser", no_argument, &debug_parser, 1 }, 
        { "gc-stress", no_argument, &gc_stress, 1 }, 
        { "help", no_argument, &help, 1 }, 
        { "init-heap-size", required_argument, NULL, 'i' }, 
        { "threshold", required_argument, NULL, 't' }, 
        { "version", no_argument, NULL, 'v' }, 
        { 0, 0, 0, 0 }, 
    };
#define USAGE       usage()
#define ERROR(msg)  do { \
    fprintf(stderr, "%s\n", msg); \
    USAGE; \
    return -1; \
} while (0)
    char c = 0;
    while ((c = getopt_long(argc, argv, "", options, NULL)) != -1) {
        switch (c) {
        case 0:
            break;
        case 'i':
            init_heap_size = parse_size(optarg);
            break;
        case 't':
            threshold = parse_size(optarg);
            break;
        case 'v':
            print_version();
            exit(0);
            break;
        default:
            ERROR("Unknown option.");
            break;
        }
    }

    if (help) {
        USAGE;
        return 0;
    }
#undef ERROR
#undef USAGE

#define ERROR(msg)  do { \
    fprintf(stderr, "%s\n", msg); \
    return -2; \
} while (0)
    YogEnv env;
    env.vm = NULL;
    env.thread = YUNDEF;

    YogVM vm;
    YogVM_init(&vm);
    vm.gc_stress = gc_stress;
    env.vm = &vm;

    YogThread dummy_thread_body;
    YogVal dummy_thread = PTR2VAL(&dummy_thread_body);
    YogThread_initialize(&env, dummy_thread, YUNDEF);
#if defined(GC_BDW)
    GC_INIT();
    YogThread_config_bdw(&env, dummy_thread);
#elif defined(GC_COPYING)
    YogThread_config_copying(&env, dummy_thread, init_heap_size);
    YogCopying_allocate_heap(&env, PTR_AS(YogThread, dummy_thread)->heap);
#elif defined(GC_MARK_SWEEP)
    if (gc_stress) {
        threshold = 0;
    }
    YogThread_config_mark_sweep(&env, dummy_thread, threshold);
#elif defined(GC_MARK_SWEEP_COMPACT)
    if (gc_stress) {
        threshold = 0;
    }
#   define CHUNK_SIZE  (16 * 1024 * 1024)
    YogThread_config_mark_sweep_compact(&env, dummy_thread, CHUNK_SIZE, threshold);
#   undef CHUNK_SIZE
#elif defined(GC_GENERATIONAL)
#   define CHUNK_SIZE  (16 * 1024 * 1024)
#   define TENURE       32
    if (!YogMarkSweepCompact_install_sigsegv_handler(&env)) {
        ERROR("failed installing SIGSEGV handler");
    }
    YogThread_config_generational(&env, dummy_thread, init_heap_size, CHUNK_SIZE, threshold, TENURE);
    YogGenerational_allocate_heap(&env, PTR_AS(YogThread, dummy_thread)->heap);
#   undef TENURE
#   undef CHUNK_SIZE
#endif
    env.thread = dummy_thread;
    YogVal main_thread = YogThread_new(&env);
    memcpy(VAL2PTR(main_thread), VAL2PTR(dummy_thread), sizeof(YogThread));
    env.thread = main_thread;
    YogVM_set_main_thread(&env, &vm, main_thread);

#define GUARD_ENV(env) \
    YogLocals env_guard; \
    env_guard.num_vals = 1; \
    env_guard.size = 1; \
    env_guard.vals[0] = &((env).thread); \
    env_guard.vals[1] = NULL; \
    env_guard.vals[2] = NULL; \
    env_guard.vals[3] = NULL; \
    env_guard.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(&env, env_guard);
    GUARD_ENV(env);
#undef GUARD_ENV

    do {
        YogVM_boot(&env, env.vm);
        YogVM_configure_search_path(&env, env.vm, argv[0]);

        const char* filename = NULL;
        if (optind < argc) {
            filename = argv[optind];
        }
        YogEval_eval_file(&env, filename, "__main__");
    } while (0);

    YogVM_remove_thread(&env, env.vm, env.thread);

    YogVM_wait_finish(&env, env.vm);
    YogVM_delete(&env, env.vm);

    return 0;
#undef ERROR
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
