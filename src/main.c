#include <stdio.h>
#include "yog/yog.h"

extern FILE* yyin;
int yyparse();

int 
main(int argc, char* argv[]) 
{
#define INIT_HEAP_SIZE  (1)
    YogVm* vm = YogVm_new(INIT_HEAP_SIZE);
#undef INIT_HEAP_SIZE
    YogEnv env = { vm };
    vm->id2name = YogTable_new_symbol_table(&env);
    vm->name2id = YogTable_new_string_table(&env);

    YogKlass* obj_klass = YogKlass_new(&env, NULL);
    YogKlass* klass_klass = YogKlass_new(&env, obj_klass);
    YOGBASICOBJ(obj_klass)->klass = klass_klass;
    YOGBASICOBJ(klass_klass)->klass = klass_klass;
    vm->obj_klass = obj_klass;
    vm->klass_klass = klass_klass;

#if 0
    YogVm_alloc_obj(&env, env.vm, OBJ_ARRAY, 1024);
    YogTable_new_symbol_table(&env);
    YogArray_new(&env);
#endif

    Yog_set_parsing_env(&env);
    if (1 < argc) {
        yyin = fopen(argv[1], "r");
    }
    else {
        yyin = stdin;
    }
    yyparse();
    if (1 < argc) {
        fclose(yyin);
    }

    YogArray* stmts = Yog_get_parsed_tree();
    YogCode* code = Yog_compile_module(&env, stmts);
    YogThread* th = YogThread_new(&env);
    YogThread_eval_code(&env, th, code);

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
