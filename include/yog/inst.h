/**
 * This file was generated by "tools/inst.py src/insts.def" automatically.
 * DO NOT TOUCH!!
 */
#ifndef __YOG_INST_H__
#define __YOG_INST_H__

#include "yog/opcodes.h"
#include "yog/yog.h"

struct YogInst {
    struct YogInst* next;

    enum InstType type;
    enum OpCode opcode;
    union {
        pc_t pos;

        struct {
            ID id;
        } load_special;
        struct {
        } pop;
        struct {
            uint8_t index;
        } push_const;
        struct {
            ID method;
            uint8_t argc;
            uint8_t kwargc;
            uint8_t blockargc;
            uint8_t varargc;
            uint8_t varkwargc;
        } call_method;
        struct {
            ID id;
        } store_pkg;
        struct {
            uint8_t index;
        } store_local;
        struct {
            ID command;
            uint8_t argc;
            uint8_t kwargc;
            uint8_t blockargc;
            uint8_t varargc;
            uint8_t varkwargc;
        } call_command;
        struct {
        } make_package_method;
        struct {
            uint8_t argc;
            uint8_t kwargc;
            uint8_t blockargc;
            uint8_t varargc;
            uint8_t varkwargc;
        } call_function;
        struct {
            ID id;
        } load_pkg;
        struct {
            uint8_t index;
        } load_local;
        struct {
            struct YogInst* dest;
        } jump;
        struct {
            struct YogInst* dest;
        } jump_if_false;
        struct {
        } dup;
        struct {
        } make_package_block;
        struct {
        } make_klass;
        struct {
        } make_method;
        struct {
        } push_self_name;
        struct {
        } ret;
    } u;
};

#define INST_OPCODE(inst)   ((inst)->opcode)
#define LABEL_POS(inst)     ((inst)->u.pos)

#define LOAD_SPECIAL_ID(inst) ((inst)->u.load_special.id)
#define PUSH_CONST_INDEX(inst) ((inst)->u.push_const.index)
#define CALL_METHOD_METHOD(inst) ((inst)->u.call_method.method)
#define CALL_METHOD_ARGC(inst) ((inst)->u.call_method.argc)
#define CALL_METHOD_KWARGC(inst) ((inst)->u.call_method.kwargc)
#define CALL_METHOD_BLOCKARGC(inst) ((inst)->u.call_method.blockargc)
#define CALL_METHOD_VARARGC(inst) ((inst)->u.call_method.varargc)
#define CALL_METHOD_VARKWARGC(inst) ((inst)->u.call_method.varkwargc)
#define STORE_PKG_ID(inst) ((inst)->u.store_pkg.id)
#define STORE_LOCAL_INDEX(inst) ((inst)->u.store_local.index)
#define CALL_COMMAND_COMMAND(inst) ((inst)->u.call_command.command)
#define CALL_COMMAND_ARGC(inst) ((inst)->u.call_command.argc)
#define CALL_COMMAND_KWARGC(inst) ((inst)->u.call_command.kwargc)
#define CALL_COMMAND_BLOCKARGC(inst) ((inst)->u.call_command.blockargc)
#define CALL_COMMAND_VARARGC(inst) ((inst)->u.call_command.varargc)
#define CALL_COMMAND_VARKWARGC(inst) ((inst)->u.call_command.varkwargc)
#define CALL_FUNCTION_ARGC(inst) ((inst)->u.call_function.argc)
#define CALL_FUNCTION_KWARGC(inst) ((inst)->u.call_function.kwargc)
#define CALL_FUNCTION_BLOCKARGC(inst) ((inst)->u.call_function.blockargc)
#define CALL_FUNCTION_VARARGC(inst) ((inst)->u.call_function.varargc)
#define CALL_FUNCTION_VARKWARGC(inst) ((inst)->u.call_function.varkwargc)
#define LOAD_PKG_ID(inst) ((inst)->u.load_pkg.id)
#define LOAD_LOCAL_INDEX(inst) ((inst)->u.load_local.index)
#define JUMP_DEST(inst) ((inst)->u.jump.dest)
#define JUMP_IF_FALSE_DEST(inst) ((inst)->u.jump_if_false.dest)

typedef struct YogInst YogInst;
#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
