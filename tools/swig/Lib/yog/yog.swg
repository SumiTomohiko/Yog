
%insert(runtime) %{
#include "yog/yog.h"
#include "yog/array.h"
#include "yog/package.h"

struct WrapperDef {
    const char* name;
    void* f;
};

typedef struct WrapperDef WrapperDef;
%}

%typemap(in) int {
    $1 = VAL2INT($input);
}

%typemap(out) int {
    $result = INT2VAL($1);
}

%init %{
void
SWIG_init(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);

    WrapperDef* pf = functions;
    while (pf->name != NULL) {
        YogPackage_define_function(env, pkg, pf->name, pf->f);
        pf++;
    }

    RETURN_VOID(env);
}
%}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
