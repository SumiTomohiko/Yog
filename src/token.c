
#include "yog/token.h"
#include "parser.h"

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogToken* token = ptr;
    switch (token->type) {
    case TK_NUMBER: /* FALLTHRU */
    case TK_REGEXP: /* FALLTHRU */
    case TK_STRING: /* FALLTHRU */
        token->u.val = YogVal_keep(env, token->u.val, keeper);
        break;
    default:
        break;
    }
}

YogVal 
YogToken_new(YogEnv* env) 
{
    YogToken* token = ALLOC_OBJ(env, keep_children, NULL, YogToken);
    token->type = 0;
    token->u.val = YUNDEF;
    token->lineno = 0;

    return PTR2VAL(token);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
