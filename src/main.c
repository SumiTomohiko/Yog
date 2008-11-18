#include "yog/parser.h"
#include "yog/yog.h"

int 
main(int argc, char* argv[]) 
{
#define INIT_HEAP_SIZE  (1)
    YogVm* vm = YogVm_new(INIT_HEAP_SIZE);
#undef INIT_HEAP_SIZE
    YogEnv env;
    env.vm = vm;
    env.th = NULL;
    YogVm_boot(&env, vm);

#if 0
    YogVm_alloc_obj(&env, env.vm, OBJ_ARRAY, 1024);
    YogTable_new_symbol_table(&env);
    YogArray_new(&env);
#endif

    YogParser* parser = YogParser_new(&env);
    const char* filename = NULL;
    if (1 < argc) {
        filename = argv[1];
    }
    YogArray* stmts = YogParser_parse_file(&env, parser, filename);

    YogCode* code = Yog_compile_module(&env, stmts);

    YogThread* th = YogThread_new(&env);
    env.th = th;
    YogPackage* pkg = YogPackage_new(&env);
    YogThread_eval_package(&env, th, pkg, code);

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
