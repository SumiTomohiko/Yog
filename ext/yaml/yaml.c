#include "yog/config.h"
#include <stdlib.h>
#include <string.h>
#include "syck.h"
#include "syck_st.h"
#include "yog/array.h"
#include "yog/dict.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/float.h"
#include "yog/get_args.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogIndirectPointer*
lookup_ptr(YogEnv* env, SyckParser* p, SYMID oid)
{
    /* Avoid strict aliasing rule. Do you know better way? */
    union {
        YogIndirectPointer* ptr;
        char* pc;
    } dummy;
    dummy.ptr = NULL;
    syck_lookup_sym(p, oid, &dummy.pc);
    YOG_ASSERT(env, dummy.ptr != NULL, "pointer is NULL");
    return dummy.ptr;
}

static SYMID
parse(SyckParser* p, SyckNode* node)
{
    YogEnv* env = (YogEnv*)p->bonus;
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
                YogIndirectPointer* ptr = lookup_ptr(env, p, oid);
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
                YogIndirectPointer* key = lookup_ptr(env, p, key_oid);
                SYMID val_oid = syck_map_read(node, map_value, i);
                YogIndirectPointer* val = lookup_ptr(env, p, val_oid);
                YogDict_set(env, o, key->val, val->val);
            }
        }
        break;
    default:
        YOG_BUG(env, "invalid kind (0x%08x)", node->kind);
        break;
    }

    YogIndirectPointer* ptr = YogVM_alloc_indirect_ptr(env, env->vm, o);
    SYMID oid = syck_add_sym(p, (char*)ptr);

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
free_parser(SyckParser* p)
{
    st_foreach(p->syms, free_indirect_ptr, p->bonus);
    syck_free_parser(p);
}

static void
err_handler(SyckParser* p, const char* msg)
{
    char* pc = p->cursor;
    while ((*pc != '\0') && (*pc != '\n')) {
        pc++;
    }
    *pc = '\0';

    YogEnv* env = (YogEnv*)p->bonus;
    int linect = p->linect;
    int col = p->cursor - p->lineptr;
    size_t len = strlen(p->lineptr);
    char* line = (char*)YogSysdeps_alloca(sizeof(char) * (len + 1));
    memcpy(line, p->lineptr, len + 1);

    free_parser(p);

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
        YogError_raise_TypeError(env, "yaml must be String, not %C", yaml);
    }

    SyckParser* p = syck_new_parser();
    syck_parser_set_root_on_error(p, YNIL);
    syck_parser_handler(p, parse);
    syck_parser_error_handler(p, err_handler);
    syck_parser_str(p, STRING_CSTR(yaml), YogString_size(env, yaml), NULL);
    p->bonus = (void*)env;
    SYMID root = syck_parse(p);
    obj = lookup_ptr(env, p, root)->val;
    free_parser(p);

    RETURN(env, obj);
}

struct EmitterExtra {
    YogEnv* env;
    YogVal* buf;
};

typedef struct EmitterExtra EmitterExtra;

static void
emit_str(YogEnv* env, SyckEmitter* e, YogVal obj)
{
    SAVE_ARG(env, obj);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogEval_call_method0(env, obj, "to_s");
    uint_t size = YogString_size(env, s);
    syck_emit_scalar(e, "str", scalar_none, 0, 0, 0, STRING_CSTR(s), size);

    RETURN_VOID(env);
}

static void
emit(SyckEmitter* e, st_data_t data)
{
    EmitterExtra* extra = (EmitterExtra*)e->bonus;
    YogEnv* env = extra->env;
    SAVE_LOCALS(env);
    YogVal obj = (YogVal)data;
    YogVal s = YUNDEF;
    YogVal iter = YUNDEF;
    YogVal key = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS5(env, obj, s, iter, key, val);

    if (IS_NIL(obj)) {
        syck_emit_scalar(e, "null", scalar_none, 0, 0, 0, "", 0);
    }
    else if (IS_BOOL(obj)) {
        const char* s = IS_TRUE(obj) ? "true" : "false";
        syck_emit_scalar(e, "boolean", scalar_none, 0, 0, 0, s, strlen(s));
    }
    else if (IS_FIXNUM(obj)) {
        char buf[21];   /* 64bit including a sign needs at most 21bytes */
        snprintf(buf, array_sizeof(buf), "%d", VAL2INT(obj));
        syck_emit_scalar(e, "number", scalar_none, 0, 0, 0, buf, strlen(buf));
    }
    else if (IS_SYMBOL(obj)) {
        emit_str(env, e, obj);
    }
    else if (!IS_PTR(obj)) {
        YOG_BUG(env, "invalid object (0x%08x)", obj);
    }
    else if (BASIC_OBJ_TYPE(obj) == TYPE_STRING) {
        const char* s = STRING_CSTR(obj);
        uint_t size = YogString_size(env, obj);
        syck_emit_scalar(e, "str", scalar_none, 0, 0, 0, s, size);
    }
    else if (BASIC_OBJ_TYPE(obj) == TYPE_ARRAY) {
        syck_emit_seq(e, "seq", seq_none);
        uint_t size = YogArray_size(env, obj);
        uint_t i;
        for (i = 0; i < size; i++) {
            syck_emit_item(e, (st_data_t)YogArray_at(env, obj, i));
        }
        syck_emit_end(e);
    }
    else if (BASIC_OBJ_TYPE(obj) == TYPE_DICT) {
        syck_emit_map(e, "map", map_none);
        iter = YogDict_get_iterator(env, obj);
        while (YogDictIterator_next(env, iter)) {
            key = YogDictIterator_current_key(env, iter);
            syck_emit_item(e, (st_data_t)key);
            val = YogDictIterator_current_value(env, iter);
            syck_emit_item(e, (st_data_t)val);
        }
        syck_emit_end(e);
    }
    else {
        emit_str(env, e, obj);
    }

    RETURN_VOID(env);
}

static void
output(SyckEmitter* e, const char* str, long len)
{
    EmitterExtra* extra = (EmitterExtra*)e->bonus;
    YogEnv* env = extra->env;
    SAVE_LOCALS(env);
    YogVal s = YUNDEF;
    YogVal enc = YUNDEF;
    PUSH_LOCALS2(env, s, enc);

    enc = YogEncoding_get_utf8(env);
    s = YogString_from_range(env, enc, str, str + len - 1);
    YogString_append(env, *extra->buf, s);

    RETURN_VOID(env);
}

static YogVal
dump_to_string(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    YogVal buf = YUNDEF;
    PUSH_LOCALS2(env, obj, buf);
    YogCArg params[] = { { "obj", &obj }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "dump_to_string", params, args, kw);

    buf = YogString_new(env);
    EmitterExtra extra = { env, &buf };
    SyckEmitter* emitter = syck_new_emitter();
    emitter->bonus = (void*)&extra;
    syck_emitter_handler(emitter, emit);
    syck_output_handler(emitter, output);
    syck_emit(emitter, (st_data_t)obj);
    syck_emitter_flush(emitter, 0);
    syck_free_emitter(emitter);

    RETURN(env, buf);
}

YogVal
YogInit_yaml(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = YogPackage_new(env);
    YogPackage_define_function(env, pkg, "load_string", load_string);
    YogPackage_define_function(env, pkg, "dump_to_string", dump_to_string);

    RETURN(env, pkg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
