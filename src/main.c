#include <getopt.h>
#include "yog/parser.h"
#include "yog/yog.h"

int 
main(int argc, char* argv[]) 
{
    int always_gc = 0;
    struct option options[] = {
        { "always-gc", no_argument, &always_gc, 1 }, 
        { 0, 0, 0, 0 }, 
    };
    getopt_long(argc, argv, "", options, NULL);

#define INIT_HEAP_SIZE  (1)
    YogVm* vm = YogVm_new(INIT_HEAP_SIZE);
#undef INIT_HEAP_SIZE
    if (always_gc) {
        vm->always_gc = TRUE;
    }
    else {
        vm->always_gc = FALSE;
    }

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
    YogPackage* pkg = YogPackage_new(&env);
    YogThread_eval_package(&env, th, pkg, code);

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
