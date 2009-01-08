#ifndef __YOG_CODE_H__
#define __YOG_CODE_H__

#include "yog/arg.h"
#include "yog/yog.h"

struct YogExceptionTableEntry {
    pc_t from;
    pc_t to;
    pc_t target;
};

typedef struct YogExceptionTableEntry YogExceptionTableEntry;

struct YogExceptionTable {
    struct YogExceptionTableEntry items[0];
};

typedef struct YogExceptionTable YogExceptionTable;

struct YogLinenoTableEntry {
    pc_t pc_from;
    pc_t pc_to;
    unsigned int lineno;
};

typedef struct YogLinenoTableEntry YogLinenoTableEntry;

struct YogCode {
    struct YogArgInfo arg_info;

    unsigned int stack_size;
    unsigned int local_vars_count;
    struct YogValArray* consts;
    struct YogByteArray* insts;
    unsigned int outer_size;

    unsigned int exc_tbl_size;
    struct YogExceptionTable* exc_tbl;

    unsigned int lineno_tbl_size;
    struct YogLinenoTableEntry* lineno_tbl;

    const char* filename;
    ID klass_name;
    ID func_name;
};

typedef struct YogCode YogCode;

#include "yog/opcodes.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/code.c */
void YogCode_dump(YogEnv*, YogCode*);
YogCode* YogCode_new(YogEnv*);

/* src/code.inc */
const char* YogCode_get_op_name(OpCode);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
