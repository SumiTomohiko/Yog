#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include "gc.h"
#include "yog/array.h"
#include "yog/code.h"
#include "yog/compile.h"
#include "yog/parser.h"
#include "yog/yog.h"

static void 
usage()
{
    printf("yog [options] [file]\n");
    printf("options:\n");
    printf("  --always-gc: \n");
    printf("  --disable-gc: \n");
    printf("  --gc=[bdw|copying|mark-sweep]: \n");
    printf("  --init-heap-size=size: \n");
    printf("  --threshold=size: \n");
    printf("  --help: \n");
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
    int always_gc = 0;
    int disable_gc = 0;
    int help = 0;
#define DEFAULT_INIT_HEAP_SIZE  (1)
    size_t init_heap_size = DEFAULT_INIT_HEAP_SIZE;
#undef DEFAULT_INIT_HEAP_SIZE
#define DEFAULT_THRESHOLD   (1024)
    size_t threshold = DEFAULT_THRESHOLD;
#undef DEFAULT_THRESHOLD
    YogGcType gc_type = GC_COPYING;
    struct option options[] = {
        { "always-gc", no_argument, &always_gc, 1 }, 
        { "disable-gc", no_argument, &disable_gc, 1 },
        { "gc", required_argument, NULL, 'g' }, 
        { "help", no_argument, &help, 1 }, 
        { "init-heap-size", required_argument, NULL, 'i' }, 
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
        case 'g':
            if (strcmp(optarg, "bdw") == 0) {
                gc_type = GC_BDW;
            }
            else if (strcmp(optarg, "copying") == 0) {
                gc_type = GC_COPYING;
            }
            else if (strcmp(optarg, "mark-sweep") == 0) {
                gc_type = GC_MARK_SWEEP;
            }
            else {
                ERROR("Unknown gc type.");
            }
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
    if (always_gc && disable_gc) {
        ERROR("Can't specify always_gc and disable_gc at same time.");
    }
#undef ERROR
#undef USAGE

    YogEnv env;
    env.vm = NULL;
    env.th = NULL;

#define ERROR(msg)  do { \
    fprintf(stderr, "%s\n", msg); \
    return -2; \
} while (0)
    YogVm vm;
    YogVm_init(&vm, gc_type);
    vm.always_gc = always_gc ? TRUE : FALSE;
    vm.disable_gc = disable_gc ? TRUE : FALSE;
    env.vm = &vm;

    YogThread thread;
    YogThread_init(&env, &thread);
    vm.thread = env.th = &thread;

    switch (gc_type) {
    case GC_BDW:
        GC_INIT();
        break;
    case GC_COPYING:
        YogVm_config_copying(&env, env.vm, init_heap_size);
        break;
    case GC_MARK_SWEEP:
        YogVm_config_mark_sweep(&env, env.vm, threshold);
        break;
    default:
        ERROR("Unknown GC type.");
        break;
    }
    YogVm_initialize_memory(&env, &vm);

    YogFrame* frame = FRAME(YogCFrame_new(&env));
    thread.cur_frame = frame;
#define INIT_LOCALS_SIZE     (1)
    YogValArray* locals = YogValArray_new(&env, INIT_LOCALS_SIZE);
#undef INIT_LOCALS_SIZE
    frame = thread.cur_frame;
    frame->locals = locals;

    YogVm_boot(&env, env.vm);

    YogParser* parser = YogParser_new(&env);
    const char* filename = NULL;
    if (optind < argc) {
        filename = argv[optind];
    }
    YogArray* stmts = YogParser_parse_file(&env, parser, filename);

    YogCode* code = Yog_compile_module(&env, filename, stmts);

#if 0
    YogThread* th = YogThread_new(&env);
    env.th = th;
    env.vm->thread = th;
#endif

    YogPackage* pkg = YogPackage_new(&env);
    pkg->code = code;
    YogVm_register_package(&env, env.vm, "__main__", pkg);

    thread.cur_frame = NULL;

    YogThread_eval_package(&env, &thread, pkg);

    YogVm_delete(&env, env.vm);

    return 0;
#undef ERROR
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
