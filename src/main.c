#include <getopt.h>
#include <stdio.h>
#include "yog/parser.h"
#include "yog/yog.h"

static void 
usage(const char* cmd) 
{
    printf("%s [--always-gc] [--disable-gc] [file]\n", cmd);
}

int 
main(int argc, char* argv[]) 
{
    int always_gc = 0;
    int disable_gc = 0;
    int help = 0;
    struct option options[] = {
        { "always-gc", no_argument, &always_gc, 1 }, 
        { "disable-gc", no_argument, &disable_gc, 1 },
        { "help", no_argument, &help, 1 }, 
        { 0, 0, 0, 0 }, 
    };
    while (getopt_long(argc, argv, "", options, NULL) != -1);

#define USAGE   usage(argv[0])
    if (help) {
        USAGE;
        return 0;
    }
    if (always_gc && disable_gc) {
        printf("Can't specify always_gc and disable_gc at same time.\n");
        USAGE;
        return -1;
    }
#undef USAGE

#define INIT_HEAP_SIZE  (1)
    YogVm* vm = YogVm_new(INIT_HEAP_SIZE);
#undef INIT_HEAP_SIZE
    vm->always_gc = always_gc ? TRUE : FALSE;
    vm->disable_gc = disable_gc ? TRUE : FALSE;

    YogEnv env;
    env.vm = vm;
    env.th = NULL;
    YogVm_boot(&env, vm);

    YogParser* parser = YogParser_new(&env);
    const char* filename = NULL;
    if (optind < argc) {
        filename = argv[optind];
    }
    YogArray* stmts = YogParser_parse_file(&env, parser, filename);

    YogCode* code = Yog_compile_module(&env, filename, stmts);

    YogThread* th = YogThread_new(&env);
    env.th = th;
    env.vm->thread = th;

    YogPackage* pkg = YogPackage_new(&env);
    pkg->code = code;
    YogVm_register_package(&env, env.vm, "__main__", pkg);
    YogThread_eval_package(&env, th, pkg);

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
