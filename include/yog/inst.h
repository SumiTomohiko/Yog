/**
 * This file was generated by "tools/inst.py src/insts.def" automatically.
 * DO NOT TOUCH!!
 */
#ifndef __YOG_INSTS_H__
#define __YOG_INSTS_H__

struct YogInst {
    YOGGCOBJ_HEAD;


    union {
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
            ID command;
            uint8_t argc;
        } call_command;
    } u;
};


#define PUSH_CONST_INDEX(inst) ((inst)->u.push_const.index)
#define CALL_METHOD_METHOD(inst) ((inst)->u.call_method.method)
#define CALL_METHOD_ARGC(inst) ((inst)->u.call_method.argc)
#define STORE_PKG_ID(inst) ((inst)->u.store_pkg.id)
#define CALL_COMMAND_COMMAND(inst) ((inst)->u.call_command.command)
#define CALL_COMMAND_ARGC(inst) ((inst)->u.call_command.argc)

typedef struct YogInst YogInst;
#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
