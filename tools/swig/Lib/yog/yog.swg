
%runtime %{
#include "yog/yog.h"
%}

%init %{
void
SWIG_init(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    RETURN_VOID(env);
}
%}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
