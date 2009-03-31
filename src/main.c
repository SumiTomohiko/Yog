#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include "gc.h"
#include "yog/code.h"
#include "yog/compile.h"
#include "yog/error.h"
#include "yog/parser.h"
#include "yog/yog.h"

static void 
usage()
{
    printf("yog [options] [file]\n");
    printf("options:\n");
    printf("  --debug-parser: \n");
    printf("  --disable-gc: \n");
    printf("  --gc-stress: \n");
    printf("  --help: \n");
    printf("  --init-heap-size=size: \n");
    printf("  --print-gc-stat: \n");
    printf("  --threshold=size: \n");
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
    int disable_gc = 0;
    int help = 0;
#define DEFAULT_INIT_HEAP_SIZE  (16 * 1024 * 1024)
    size_t init_heap_size = DEFAULT_INIT_HEAP_SIZE;
#undef DEFAULT_INIT_HEAP_SIZE
#define DEFAULT_THRESHOLD   (1024 * 1024)
    size_t threshold = DEFAULT_THRESHOLD;
#undef DEFAULT_THRESHOLD
    int print_gc_stat = 0;
    struct option options[] = {
        { "debug-parser", no_argument, &debug_parser, 1 }, 
        { "disable-gc", no_argument, &disable_gc, 1 },
        { "gc-stress", no_argument, &gc_stress, 1 }, 
        { "help", no_argument, &help, 1 }, 
        { "init-heap-size", required_argument, NULL, 'i' }, 
        { "print-gc-stat", no_argument, &print_gc_stat, 1 }, 
        { "threshold", required_argument, NULL, 't' }, 
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
        default:
            ERROR("Unknown option.");
            break;
        }
    }

    if (help) {
        USAGE;
        return 0;
    }
    if (gc_stress && disable_gc) {
        ERROR("Can't specify gc_stress and disable_gc at same time.");
    }
#undef ERROR
#undef USAGE

#define ERROR(msg)  do { \
    fprintf(stderr, "%s\n", msg); \
    return -2; \
} while (0)
    YogVm vm;
    YogVm_init(&vm);
    vm.gc_stress = gc_stress ? TRUE : FALSE;
    vm.disable_gc = disable_gc ? TRUE : FALSE;
    vm.gc_stat.print = print_gc_stat ? TRUE : FALSE;

    YogEnv env;
    env.vm = &vm;
    env.th = NULL;
#if defined(GC_BDW)
    GC_INIT();
#elif defined(GC_COPYING)
    YogVm_config_copying(&env, env.vm, init_heap_size);
#elif defined(GC_MARK_SWEEP)
    if (gc_stress) {
        threshold = 0;
    }
    YogVm_config_mark_sweep(&env, env.vm, threshold);
#elif defined(GC_MARK_SWEEP_COMPACT)
    if (gc_stress) {
        threshold = 0;
    }
#   define CHUNK_SIZE  (16 * 1024 * 1024)
    YogVm_config_mark_sweep_compact(&env, env.vm, CHUNK_SIZE, threshold);
#   undef CHUNK_SIZE
#elif defined(GC_GENERATIONAL)
#   define CHUNK_SIZE  (16 * 1024 * 1024)
#   define TENURE       32
    YogVm_config_generational(&env, env.vm, init_heap_size, CHUNK_SIZE, threshold, TENURE);
#   undef TENURE
#   undef CHUNK_SIZE
#else
#   error "unknown GC type"
#endif

    do {
        YogThread thread;
        YogThread_initialize(&env, &thread);
        vm.thread = env.th = &thread;

        YogVal stmts = YUNDEF;
        YogVal code = YUNDEF;
        YogVal pkg = YUNDEF;
        PUSH_LOCALS3(&env, stmts, code, pkg);

        YogVm_boot(&env, env.vm);

        const char* filename = NULL;
        if (optind < argc) {
            filename = argv[optind];
        }
        stmts = YogParser_parse_file(&env, filename, debug_parser != 0);
        code = YogCompiler_compile_module(&env, filename, stmts);

        pkg = YogPackage_new(&env);
        OBJ_AS(YogPackage, pkg)->code = VAL2PTR(code);
        YogVm_register_package(&env, env.vm, "__main__", pkg);
        YogThread_eval_package(&env, &thread, pkg);

        YogThread_finalize(&env, &thread);

        POP_LOCALS(&env);
    } while (0);

    if (vm.gc_stat.print) {
        printf("GC duration total: %u[usec]\n", vm.gc_stat.duration_total);
        printf("allocation #: %u\n", vm.gc_stat.num_alloc);
    }

    YogVm_delete(&env, env.vm);

    return 0;
#undef ERROR
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
