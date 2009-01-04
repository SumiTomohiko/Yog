#! python
# -*- coding: utf-8 -*-

from StringIO import StringIO
from datetime import datetime
from os.path import join
from re import finditer
from string import Template
from sys import argv
import re

class Operand(object):

    def __init__(self, type, name):
        self.type = type
        self.name = name

    def __repr__(self):
        return "%(type)s %(name)s" % { "type": self.type, "name": self.name }

class Inst(object):

    def __init__(self, name, lineno):
        self.name = name
        self.operands = []
        self.pop_values = []
        self.pop_size = "0"
        self.push_values = []
        self.push_size = "0"
        self.codes = []
        self.lineno = lineno

    def __repr__(self):
        s = StringIO()
        s.write("inst %(name)s\n" % { "name": self.name })
        s.write("(%(operands)s)\n" % { 
                "operands": ", ".join([`op` for op in self.operands]) })
        s.write("(%(pop_values)s)\n" % { "pop_values": 
                ", ".join([pop_value for pop_value in self.pop_values]) })
        s.write("(%(push_values)s)\n" % { "push_values": 
                ", ".join([push_value for push_value in self.push_values]) })
        s.write("{\n")
        s.write("\n".join([code  for code in self.codes]))
        s.write("\n}\n")

        return s.getvalue()

class CodeGenerator(object):

    def __init__(self):
        self.lineno = -1
        self.lines = None
        self.insts = []

    def readline(self):
        self.lineno += 1
        line = self.lines[self.lineno].rstrip()
        return line

    comment_start = re.compile(r"\A/\*")
    comment_end = re.compile(r"\*/\Z")
    inst_start = re.compile(r"\Ainst\s+(?P<name>\w+)\Z")
    inst_end = re.compile(r"\A}\Z")

    def split_values(self, s):
        left = s.find("(")
        right = s.find(")")
        return [t.strip() for t in s[left + 1:right].split(",") 
                if (0 < len(t.strip())) and (t.strip() != "...")]

    def parse_depth(self, s):
        t = "depth:"
        n = s.find(t)
        if n < 0:
            return []
        u = s[n + len(t):]
        return finditer(r"(?P<star>\*)|(?P<plus>\+)|(?P<name>[a-z]+)|(?P<number>[0-9]+)", u)

    def parse_inst(self, name):
        inst = Inst(name, self.lineno)

        line = self.readline()
        operands = self.split_values(line)
        for operand in operands:
            type = " ".join(operand.split()[:-1])
            name = operand.split()[-1]
            inst.operands.append(Operand(type, name))

        line = self.readline()
        inst.pop_values.extend(self.split_values(line))
        inst.pop_size = self.parse_depth(line)

        line = self.readline()
        inst.push_values.extend(self.split_values(line))
        inst.push_size = self.parse_depth(line)

        self.readline() # Skip "{"

        while True:
            line = self.readline()
            if self.inst_end.search(line):
                break
            inst.codes.append(line)

        self.insts.append(inst)

    def parse_comment(self):
        while True:
            line = self.readline()
            if self.comment_end.search(line):
                break

    def parse_def(self):
        self.insts = []

        while True:
            try:
                line = self.readline()
            except IndexError:
                break
            else:
                patterns = {
                        self.comment_start: self.parse_comment,
                        self.inst_start: self.parse_inst, 
                        }
                for pattern, proc in patterns.items():
                    m = pattern.search(line)
                    if m:
                        proc(**m.groupdict())
                        break

    def open(self, filename):
        fp = open(filename)
        try:
            self.lines = fp.readlines()
        finally:
            fp.close()

        self.lineno = 0

    def substitute_template(self, tmpl_file, kw):
        fp = open(tmpl_file)
        try:
            s = fp.read()
        finally:
            fp.close()
        return Template(s).substitute(kw)

    def make_attention(self):
        cmd = " ".join(argv)
        return """/**
 * This file was generated by "%(cmd)s" automatically.
 * DO NOT TOUCH!!
 */
""" % { "cmd": cmd }

    def write_file(self, filename, s):
        fp = open(filename, "w")
        try:
            fp.write(self.make_attention())
            fp.write(s)
        finally:
            fp.close()

    def gen_opcodes_h(self, opcodes_h, opcodes_h_tmpl):
        opcodes = StringIO()
        for i, inst in enumerate(self.insts):
            s = "    OP(%(name)s) = %(i)d, \n" \
                    % { "name": inst.name.upper(), "i": i }
            opcodes.write(s)
        kw = { "opcodes": opcodes.getvalue() }
        s = self.substitute_template(opcodes_h_tmpl, kw)
        self.write_file(opcodes_h, s)

    def get_c_footer(self):
        return """/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
"""

    def gen_thread_inc(self, def_, thread_inc):
        lineno = len(self.make_attention().split("\n")) - 1
        inc = StringIO()
        for inst in self.insts:
            inc.write("""
#line %(lineno)d \"%(inc)s\"""" % { "lineno": lineno + 2, "inc": thread_inc })
            lineno += 1

            s = """
    case OP(%(name)s):
        {""" % { "name": inst.name.upper() }
            lineno += len(s.split("\n")) - 1
            inc.write(s)

            declared_names = set()
            for operand in inst.operands:
                name = operand.name
                if name in declared_names:
                    raise Exception("%(name)s is used." % { "name": name })

                s = """
            YOG_ASSERT(env, PC < YogByteArray_size(env, CODE->insts), "pc is over code length.");
            %(type)s %(name)s = *((%(type)s*)&CODE->insts->items[PC]);
            PC += sizeof(%(type)s);""" % { "type": operand.type, "name": name }
                lineno += len(s.split("\n")) - 1
                inc.write(s)
                declared_names.add(name)

            for pop_value in inst.pop_values:
                if pop_value not in declared_names:
                    if (0 < len(inst.codes)) or (0 < len(inst.push_values)):
                        s = """
            YogVal %(name)s = POP();""" % { "name": pop_value }
                    else:
                        s = """
            POP();"""
                    lineno += len(s.split("\n")) - 1
                    inc.write(s)
                    declared_names.add(pop_value)

            for push_value in inst.push_values:
                if push_value not in declared_names:
                    s = """
            YogVal %(name)s = YogVal_nil();""" % { "name": push_value }
                    lineno += len(s.split("\n")) - 1
                    inc.write(s)
                    declared_names.add(push_value)

            s = """
#line %(lineno)d \"%(def)s\"""" % { "lineno": inst.lineno + 6, "def": def_ }
            lineno += len(s.split("\n")) - 1
            inc.write(s)

            inc.write("\n")
            in_macro = False
            for line in [s.rstrip() for s in inst.codes]:
                if line.startswith("#") or in_macro:
                    inc.write(line)
                    if line.endswith("\\"):
                        in_macro = True
                    else:
                        in_macro = False
                else:
                    inc.write(" " * 8)
                    inc.write(line)
                    in_macro = False
                lineno += 1
                inc.write("\n")

            inc.write("""
#line %(lineno)d \"%(inc)s\"""" % { "lineno": lineno + 2, "inc": thread_inc })
            lineno += 1

            for push_value in inst.push_values:
                s = """
            PUSH(%(name)s);""" % { "name": push_value }
                lineno += len(s.split("\n")) - 1
                inc.write(s)

            s = """
            break;
        }"""
            lineno += len(s.split("\n")) - 1
            inc.write(s)

        inc.write("\n" + self.get_c_footer())
        self.write_file(thread_inc, inc.getvalue())

    def type_name2func_name(self, name):
        name = name.lower().replace(" ", "_")
        suffix = "_t"
        if name.endswith(suffix):
            name = name[:- len(suffix)]
        return name

    def gen_inst_c(self, inst_c, inst_c_tmpl):
        inst2size = StringIO()
        for inst in self.insts:
            inst2size.write(" " * 8 + "sizeof(uint8_t)")
            for i, operand in enumerate(inst.operands):
                inst2size.write(
                        " + sizeof(%(type)s)" % { "type": operand.type })
            inst2size.write(", /* %(name)s */\n" % { "name": inst.name })

        s = self.substitute_template(
                inst_c_tmpl, { "inst2size": inst2size.getvalue() })
        with open(inst_c, "w") as f:
            f.write(self.make_attention())
            f.write(s)

    def stack_depth2c(self, name, iter, values):
        c = ""
        for m in iter:
            if m.group("star"):
                s = m.group("star")
            elif m.group("plus"):
                s = m.group("plus")
            elif m.group("name"):
                s = "%(inst)s_%(name)s(inst)" % { "name": m.group("name").upper(), "inst": name.upper() }
            elif m.group("number"):
                s = m.group("number")
            else:
                s = ""
            if 0 < len(s):
                c += " " + s
        if len(c) == 0:
            c = " " + str(len(values))

        return c

    def gen_compile_inc(self, compile_inc, compile_inc_tmpl):
        compile_data = StringIO()
        for inst in self.insts:
            compile_data.write("""
static void 
CompileData_add_%(inst)s(YogEnv* env, CompileData* data, unsigned int lineno""" % { "inst": inst.name })
            for operand in inst.operands:
                compile_data.write(", %(type)s %(name)s" % { "type": self.type_name2data_type(operand.type), "name": operand.name })
            compile_data.write(""")
{
    FRAME_DECL_LOCAL(env, data_idx, PTR2VAL(data));

    YogInst* inst = Inst_new(env, lineno);
    inst->type = INST_OP;
    INST_OPCODE(inst) = OP(%(name)s);
""" % { "name": inst.name.upper() })
            for operand in inst.operands:
                compile_data.write("""
    %(inst)s_%(operand)s(inst) = %(name)s;""" % { "inst": inst.name.upper(), "operand": operand.name.upper(), "name": operand.name })
            compile_data.write("""

    FRAME_LOCAL_PTR(env, data, data_idx);
    add_inst(data, inst);
}
""")

        insts2bin = StringIO()
        for inst in self.insts:
            insts2bin.write("""
                case OP(%(name)s):
                    {
                        UPDATE_CODE;
                        YogBinary_push_uint8(env, code, OP(%(name)s));""" % { "name": inst.name.upper() })
            for operand in inst.operands:
                inst_attr = "%(inst)s_%(name)s(inst)" % { "inst": inst.name.upper(), "name": operand.name.upper() }
                if operand.type == "pc_t":
                    inst_attr = inst_attr + "->pc"
                insts2bin.write("""
                        UPDATE_CODE;
                        UPDATE_INST;
                        YogBinary_push_%(type)s(env, code, %(inst_attr)s);""" % { "type": self.type_name2func_name(operand.type), "inst_attr": inst_attr })
            insts2bin.write("""
                        break;
                    }""")

        count_stack_size = StringIO()
        for inst in self.insts:
            pop_size = self.stack_depth2c(inst.name, inst.pop_size, inst.pop_values)
            push_size = self.stack_depth2c(inst.name, inst.push_size, inst.push_values)
            count_stack_size.write("""
                case OP(%(name)s):
                    {
                        pop_size =%(pop_size)s;
                        push_size =%(push_size)s;
                        break;
                    }""" % { "name": inst.name.upper(), "pop_size": pop_size, "push_size": push_size })

        s = self.substitute_template(
                compile_inc_tmpl, 
                { "compile_data": compile_data.getvalue(), 
                    "insts2bin": insts2bin.getvalue(), 
                    "count_stack_size": count_stack_size.getvalue() })
        with open(compile_inc, "w") as f:
            f.write(self.make_attention())
            f.write(s)

    def tmpl2file(self, tmpl, kw, filename):
        s = self.substitute_template(tmpl, kw)
        with open(filename, "w") as f:
            f.write(self.make_attention())
            f.write(s)

    def gen_code_inc(self, code_inc, code_inc_tmpl):
        op_names = StringIO()
        for inst in self.insts:
            op_names.write("""
        "%(name)s", """ % { "name": inst.name.upper() })
        kw = { "op_names": op_names.getvalue() }
        self.tmpl2file(code_inc_tmpl, kw, code_inc)

    def type_name2data_type(self, type):
        if type == "pc_t":
            return "struct YogInst*"
        else:
            return type

    def gen_inst_h(self, inst_h, inst_h_tmpl):
        structs = StringIO()
        for inst in self.insts:
            structs.write("""
        struct {""")
            for operand in inst.operands:
                structs.write("""
            %(type)s %(name)s;""" % { "type": self.type_name2data_type(operand.type), "name": operand.name })

            structs.write("""
        } %(name)s;""" % { "name": inst.name })

        macros = StringIO()
        for inst in self.insts:
            for operand in inst.operands:
                macros.write("""
#define %(inst_macro)s_%(operand_macro)s(inst) ((inst)->u.%(inst)s.%(operand)s)""" % { "inst_macro": inst.name.upper(), "operand_macro": operand.name.upper(), "inst": inst.name, "operand": operand.name })

        kw = { "structs": structs.getvalue(), "macros": macros.getvalue(), }
        self.tmpl2file(inst_h_tmpl, kw, inst_h)

    def do(self, def_, opcodes_h=None, opcodes_h_tmpl=None, thread_inc=None, 
            compile_inc=None, compile_inc_tmpl=None, code_inc=None, 
            code_inc_tmpl=None, debug=False):
        self.open(def_)
        self.parse_def()
        if debug:
            for inst in self.insts:
                print `inst`

        # Generate inst.h from inst.h.tmpl .
        include_dir = join("include", "yog")
        opcodes_h = opcodes_h or join(include_dir, "opcodes.h")
        opcodes_h_tmpl = opcodes_h_tmpl or opcodes_h + ".tmpl"
        self.gen_opcodes_h(opcodes_h, opcodes_h_tmpl)

        inst_h = join(include_dir, "inst.h")
        inst_h_tmpl = inst_h + ".tmpl"
        self.gen_inst_h(inst_h, inst_h_tmpl)

        # Generate thread.inc (no templates).
        src_dir = "src"
        thread_inc = thread_inc or join(src_dir, "thread.inc")
        self.gen_thread_inc(def_, thread_inc)

        # Generate compile.inc.
        compile_inc = compile_inc or join(src_dir, "compile.inc")
        compile_inc_tmpl = compile_inc_tmpl or compile_inc + ".tmpl"
        self.gen_compile_inc(compile_inc, compile_inc_tmpl)

        inst_c = join(src_dir, "inst.c")
        inst_c_tmpl = inst_c + ".tmpl"
        self.gen_inst_c(inst_c, inst_c_tmpl)

        # Generate code.inc.
        code_inc = code_inc or join(src_dir, "code.inc")
        code_inc_tmpl = code_inc_tmpl or code_inc + ".tmpl"
        self.gen_code_inc(code_inc, code_inc_tmpl)

if __name__ == "__main__":
    CodeGenerator().do(argv[1])

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
