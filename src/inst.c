/**
 * This file was generated by "../tools/inst.py insts.def .." automatically.
 * DO NOT TOUCH!!
 */
#include <stdint.h>
#include "yog/opcodes.h"
#include "yog/yog.h"

uint_t
Yog_get_inst_size(OpCode op)
{
    uint_t inst2size[] = {
        sizeof(uint8_t), /* pop */
        sizeof(uint8_t) + sizeof(uint8_t), /* push_const */
        sizeof(uint8_t) + sizeof(uint8_t), /* make_string */
        sizeof(uint8_t) + sizeof(ID), /* store_name */
        sizeof(uint8_t) + sizeof(uint8_t), /* store_local */
        sizeof(uint8_t), /* make_block */
        sizeof(uint8_t), /* make_block2 */
        sizeof(uint8_t), /* make_function */
        sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t), /* call_function */
        sizeof(uint8_t) + sizeof(ID), /* load_global */
        sizeof(uint8_t) + sizeof(ID), /* load_name */
        sizeof(uint8_t) + sizeof(uint8_t), /* load_local */
        sizeof(uint8_t) + sizeof(pc_t), /* jump */
        sizeof(uint8_t) + sizeof(pc_t), /* jump_if_true */
        sizeof(uint8_t) + sizeof(pc_t), /* jump_if_false */
        sizeof(uint8_t), /* dup */
        sizeof(uint8_t) + sizeof(ID), /* make_module */
        sizeof(uint8_t), /* make_class */
        sizeof(uint8_t), /* push_self_name */
        sizeof(uint8_t), /* ret */
        sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t), /* store_nonlocal */
        sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t), /* load_nonlocal */
        sizeof(uint8_t) + sizeof(ID), /* store_global */
        sizeof(uint8_t) + sizeof(ID), /* load_attr */
        sizeof(uint8_t) + sizeof(uint8_t), /* make_array */
        sizeof(uint8_t) + sizeof(uint8_t), /* make_dict */
        sizeof(uint8_t) + sizeof(uint8_t), /* make_set */
        sizeof(uint8_t), /* finish */
        sizeof(uint8_t), /* print_top */
        sizeof(uint8_t) + sizeof(ID), /* store_attr */
        sizeof(uint8_t), /* not */
        sizeof(uint8_t), /* long_return */
        sizeof(uint8_t), /* long_break */

    };

    return inst2size[op];
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
