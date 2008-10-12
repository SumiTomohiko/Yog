/**
 * This file was generated by "tools/inst.py src/insts.def" automatically.
 * DO NOT TOUCH!!
 */
#ifndef __YOG_INSTS_H__
#define __YOG_INSTS_H__
#if defined(__cplusplus)
extern "C" {
#endif

#define OP(name)  OP_##name

enum OpCode {
    OP(PUSH_CONST) = 1, 
    OP(CALL_METHOD) = 2, 
    OP(STORE_PKG) = 3, 
    OP(CALL_COMMAND) = 4, 
    OP(MAKE_FUNC) = 5, 
    OP(CALL_FUNC) = 6, 
    OP(LOAD_PKG) = 7, 
    OP(LOAD_LOCAL) = 8, 
    OP(JUMP) = 9, 
    OP(RAISE) = 10, 
    OP(RERAISE) = 11, 
    OP(DELETE_EXC) = 12, 

};

typedef enum OpCode OpCode;

#if defined(__cplusplus)
}
#endif
#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
