/**
 * This file was generated by "tools/inst.py src/insts.def" automatically.
 * DO NOT TOUCH!!
 */
#ifndef __YOG_INST_H__
#define __YOG_INST_H__

#include "yog/opcodes.h"
#include "yog/yog.h"

struct YogInst {
    YOGGCOBJ_HEAD;

    struct YogInst* next;

    enum InstType type;
    enum OpCode operand;
    union {
        pc_t pos;

        struct {
            ID id;
        } load_special;
        struct {
            uint8_t index;
        } push_const;
        struct {
            ID method;
            uint8_t argc;
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
        } call_command;
        struct {
        } make_func;
        struct {
            uint8_t argc;
        } call_func;
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
    } u;
};

#define INST_OPERAND(inst)  ((inst)->operand)
#define LABEL_POS(inst)     ((inst)->u.pos)

#define LOAD_SPECIAL_ID(inst) ((inst)->u.load_special.id)
#define PUSH_CONST_INDEX(inst) ((inst)->u.push_const.index)
#define CALL_METHOD_METHOD(inst) ((inst)->u.call_method.method)
#define CALL_METHOD_ARGC(inst) ((inst)->u.call_method.argc)
#define STORE_PKG_ID(inst) ((inst)->u.store_pkg.id)
#define STORE_LOCAL_INDEX(inst) ((inst)->u.store_local.index)
#define CALL_COMMAND_COMMAND(inst) ((inst)->u.call_command.command)
#define CALL_COMMAND_ARGC(inst) ((inst)->u.call_command.argc)
#define CALL_FUNC_ARGC(inst) ((inst)->u.call_func.argc)
#define LOAD_PKG_ID(inst) ((inst)->u.load_pkg.id)
#define LOAD_LOCAL_INDEX(inst) ((inst)->u.load_local.index)
#define JUMP_DEST(inst) ((inst)->u.jump.dest)
#define JUMP_IF_FALSE_DEST(inst) ((inst)->u.jump_if_false.dest)

typedef struct YogInst YogInst;
#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
