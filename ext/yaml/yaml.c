#include "yog/config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "syck.h"
#include "syck_st.h"
#include "yog/array.h"
#include "yog/dict.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/float.h"
#include "yog/get_args.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogIndirectPointer*
lookup_ptr(YogEnv* env, SyckParser* parser, SYMID oid)
{
    /* Avoid strict aliasing rule. Do you know better way? */
    union {
        YogIndirectPointer* ptr;
        char* pc;
    } dummy;
    dummy.ptr = NULL;
    syck_lookup_sym(parser, oid, &dummy.pc);
    YOG_ASSERT(env, dummy.ptr != NULL, "pointer is NULL");
    return dummy.ptr;
}

static SYMID
handler(SyckParser* parser, SyckNode* node)
{
    YogEnv* env = (YogEnv*)parser->bonus;
    SAVE_LOCALS(env);
    YogVal o = YUNDEF;
    YogVal enc = YUNDEF;
    PUSH_LOCALS2(env, o, enc);

    const char* type_id = node->type_id;
    switch (node->kind) {
    case syck_str_kind:
#define CMP_TYPE(s)     (strcmp(node->type_id, (s)) == 0)
        if ((type_id == NULL) || CMP_TYPE("str")) {
            enc = YogEncoding_get_utf8(env);
            const char* from = node->data.str->ptr;
            const char* to = from + node->data.str->len - 1;
            o = YogString_from_range(env, enc, from, to);
        }
        else if (CMP_TYPE("null")) {
            o = YNIL;
        }
        else if (CMP_TYPE("bool#yes")) {
            o = YTRUE;
        }
        else if (CMP_TYPE("bool#no")) {
            o = YFALSE;
        }
        else if (CMP_TYPE("int#base60")) {
            syck_str_blow_away_commas(node);

            int_t sixty = 1;
            int_t total = 0;
            char* ptr = node->data.str->ptr;
            char* end = ptr + node->data.str->len;
            while (ptr < end) {
                char* colon = end - 1;
                while ((ptr <= colon) && (*colon != ':')) {
                    colon--;
                }
                if ((ptr <= colon) && (*colon == ':')) {
                    *colon = '\0';
                }

                total += strtol(colon + 1, NULL, 10) * sixty;
                sixty *= 60;
                end = colon;
            }
            o = YogVal_from_int(env, total);
        }
        else if (CMP_TYPE("int#hex")) {
            int_t i = strtol(node->data.str->ptr, NULL, 16);
            o = YogVal_from_int(env, i);
        }
        else if (CMP_TYPE("int")) {
            int_t i = strtol(node->data.str->ptr, NULL, 10);
            o = YogVal_from_int(env, i);
        }
        else if (CMP_TYPE("int#oct")) {
            int_t i = strtol(node->data.str->ptr, NULL, 8);
            o = YogVal_from_int(env, i);
        }
        else if (CMP_TYPE("float#base60")) {
            syck_str_blow_away_commas(node);

            int_t sixty = 1;
            double total = 0.0;
            char* ptr = node->data.str->ptr;
            char* end = ptr + node->data.str->len;
            while (ptr < end) {
                char* colon = end - 1;
                while ((ptr <= colon) && (*colon != ':')) {
                    colon--;
                }
                if ((ptr <= colon) && (*colon == ':')) {
                    *colon = '\0';
                }

                total += strtod(colon + 1, NULL) * sixty;
                sixty *= 60;
                end = colon;
            }

            o = YogFloat_new(env);
            PTR_AS(YogFloat, o)->val = total;
        }
        else if (CMP_TYPE("float") || CMP_TYPE("float#fix") || CMP_TYPE("float#exp")) {
            syck_str_blow_away_commas(node);
            o = YogFloat_from_float(env, strtod(node->data.str->ptr, NULL));
        }
        else if (CMP_TYPE("float#nan")) {
            o = YogFloat_from_float(env, 0.0 / 0.0);
        }
        else if (CMP_TYPE("float#inf")) {
            o = YogFloat_from_float(env, 1.0 / 0.0);
        }
        else if (CMP_TYPE("float#neginf")) {
            o = YogFloat_from_float(env, - 1.0 / 0.0);
        }
        else {
            /**
             * TODO: The following data type are not supported.
             * * binary
             * * timestamp#iso8601
             * * timestamp#spaced
             * * timestamp#ymd
             * * timestamp
             * * merge
             * * default
             */
            enc = YogEncoding_get_utf8(env);
            const char* from = node->data.str->ptr;
            const char* to = from + node->data.str->len - 1;
            o = YogString_from_range(env, enc, from, to);
        }
        break;
#undef CMP_TYPE
    case syck_seq_kind:
        {
            uint_t size = node->data.list->idx;
            o = YogArray_of_size(env, size);
            uint_t i;
            for (i = 0; i < size; i++) {
                SYMID oid = syck_seq_read(node, i);
                YogIndirectPointer* ptr = lookup_ptr(env, parser, oid);
                YogArray_push(env, o, ptr->val);
            }
        }
        break;
    case syck_map_kind:
        {
            o = YogDict_new(env);
            uint_t size = node->data.pairs->idx;
            uint_t i;
            for (i = 0; i < size; i++) {
                SYMID key_oid = syck_map_read(node, map_key, i);
                YogIndirectPointer* key = lookup_ptr(env, parser, key_oid);
                SYMID val_oid = syck_map_read(node, map_value, i);
                YogIndirectPointer* val = lookup_ptr(env, parser, val_oid);
                YogDict_set(env, o, key->val, val->val);
            }
        }
        break;
    default:
        YOG_BUG(env, "invalid kind (0x%08x)", node->kind);
        break;
    }

    YogIndirectPointer* ptr = YogVM_alloc_indirect_ptr(env, env->vm, o);
    SYMID oid = syck_add_sym(parser, (char*)ptr);

    RETURN(env, oid);
}

static int
free_indirect_ptr(char* key, char* value, void* bonus)
{
    YogEnv* env = (YogEnv*)bonus;
    YogVM_free_indirect_ptr(env, env->vm, (YogIndirectPointer*)value);

    return ST_CONTINUE;
}

static void
free_parser(SyckParser* parser)
{
    st_foreach(parser->syms, free_indirect_ptr, parser->bonus);
    syck_free_parser(parser);
}

static void
err_handler(SyckParser* parser, const char* msg)
{
    char* pc = parser->cursor;
    while ((*pc != '\0') && (*pc != '\n')) {
        pc++;
    }
    *pc = '\0';

    YogEnv* env = (YogEnv*)parser->bonus;
    int linect = parser->linect;
    int col = parser->cursor - parser->lineptr;
    size_t len = strlen(parser->lineptr);
    char* line = (char*)alloca(sizeof(char) * (len + 1));
    memcpy(line, parser->lineptr, len + 1);

    free_parser(parser);

    YogError_raise_ArgumentError(env, "%s on line %d, col %d: \"%s\"", msg, linect, col, line);
    /* NOTREACHED */
}

static YogVal
load_string(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal yaml = YUNDEF;
    YogVal obj = YUNDEF;
    PUSH_LOCALS2(env, yaml, obj);
    YogCArg params[] = { { "yaml", &yaml }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "load_string", params, args, kw);
    if (!IS_PTR(yaml) || (BASIC_OBJ_TYPE(yaml) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "yaml must be String");
    }

    SyckParser* parser = syck_new_parser();
    syck_parser_set_root_on_error(parser, YNIL);
    syck_parser_handler(parser, handler);
    syck_parser_error_handler(parser, err_handler);
    syck_parser_str(parser, STRING_CSTR(yaml), YogString_size(env, yaml), NULL);
    parser->bonus = (void*)env;
    SYMID root = syck_parse(parser);
    /* avoid strict aliasing rule */
    union {
        char* c;
        YogIndirectPointer* p;
    } ptr;
    ptr.p = NULL;
    syck_lookup_sym(parser, root, &ptr.c);
    YOG_ASSERT(env, ptr.p != NULL, "invalid root");
    obj = ptr.p->val;
    free_parser(parser);

    RETURN(env, obj);
}

YogVal
YogInit_yaml(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = YogPackage_new(env);
    YogPackage_define_function(env, pkg, "load_string", load_string);

    RETURN(env, pkg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
