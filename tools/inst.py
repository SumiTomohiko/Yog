#! python
# -*- coding: utf-8 -*-

from datetime import datetime
from os.path import join
from string import Template
from StringIO import StringIO
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
        self.push_values = []
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
        return [s.strip() for s in s[1:-1].split(",") 
                if (0 < len(s.strip())) and (s.strip() != "...")]

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

        line = self.readline()
        inst.push_values.extend(self.split_values(line))

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
            Yog_assert(env, PC < YogByteArray_size(env, CODE->insts), "");
            %(type)s %(name)s = *((%(type)s*)&CODE->insts->items[PC]);
            PC += sizeof(%(type)s);""" % { "type": operand.type, "name": name }
                lineno += len(s.split("\n")) - 1
                inc.write(s)
                declared_names.add(name)

            for pop_value in inst.pop_values:
                if pop_value not in declared_names:
                    s = """
            Yog_assert(env, 0 < YogValArray_size(env, STACK), "");"""
                    lineno += len(s.split("\n")) - 1
                    inc.write(s)

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

    def gen_compile_inc(self, compile_inc, compile_inc_tmpl):
        compile_data = StringIO()
        for inst in self.insts:
            compile_data.write("""
static void 
CompileData_append_%(inst)s(YogEnv* env, CompileData* data""" % { "inst": inst.name })
            for operand in inst.operands:
                compile_data.write(", %(type)s %(name)s" % { "type": self.type_name2data_type(operand.type), "name": operand.name })
            compile_data.write(""")
{
    YogInst* inst = inst_new(env);
    inst->type = INST_OP;
    INST_OPERAND(inst) = OP(%(name)s);
""" % { "name": inst.name.upper() })
            for operand in inst.operands:
                compile_data.write("""
    %(inst)s_%(operand)s(inst) = %(name)s;""" % { "inst": inst.name.upper(), "operand": operand.name.upper(), "name": operand.name })
            compile_data.write("""

    append_inst(data, inst);
}
""")

        insts2bin = StringIO()
        for inst in self.insts:
            insts2bin.write("""
            case OP(%(name)s):
                {
                    YogBinary_push_uint8(env, code, OP(%(name)s));""" % { "name": inst.name.upper() })
            for operand in inst.operands:
                inst_attr = "%(inst)s_%(name)s(inst)" % { "inst": inst.name.upper(), "name": operand.name.upper() }
                if operand.type == "pc_t":
                    inst_attr = "LABEL_POS(" + inst_attr + ")"
                insts2bin.write("""
                    YogBinary_push_%(type)s(env, code, %(inst_attr)s);""" % { "type": self.type_name2func_name(operand.type), "inst_attr": inst_attr })
            insts2bin.write("""
                    break;
                }""")

        inst2size = StringIO()
        for inst in self.insts:
            inst2size.write(" " * 8 + "sizeof(uint8_t)")
            for i, operand in enumerate(inst.operands):
                inst2size.write(" + sizeof(%(type)s)" % { "type": operand.type })
            inst2size.write(", /* %(name)s */\n" % { "name": inst.name })

        s = self.substitute_template(
                compile_inc_tmpl, 
                { "compile_data": compile_data.getvalue(), 
                    "insts2bin": insts2bin.getvalue(), 
                    "inst2size": inst2size.getvalue() })
        with open(compile_inc, "w") as f:
            f.write(self.make_attention())
            f.write(s)

    def gen_code_inc(self, code_inc, types):
        inc = StringIO()
        for inst in self.insts:
            name = inst.name.upper()
            inc.write("""
    case INST(%(name)s):
        {
""" % { "name": name })
            inc.write("""
            snprintf(buffer, sizeof(buffer), "%%s(%%d)", "%(name)s", op);
            YogString_append(vm, pp, buffer);
            pc += sizeof(inst_t);
""" % { "name": name })
            for operand in inst.operands:
                inc.write("""
            assert(pc < YogByteArray_size(env, code->ops));
            %(type)s %(name)s = *((%(type)s*)&code->ops->ptr[PC]);
            pc += sizeof(%(type)s);
""" % { "type": operand.type, "name": operand.name })

                type_ = operand.type
                while True:
                    if type_ == "uint8_t":
                        format = "%u"
                        break
                    elif type_ == "int32_t":
                        format = "%d"
                        break
                    elif type_ == "unsigned long":
                        format = "%lu"
                        break
                    elif type_ == "long":
                        format = "%ld"
                        break
                    type_ = types[type_]
                inc.write("""
            snprintf(buffer, sizeof(buffer), " %(format)s", %(name)s);
            YogString_append(vm, pp, buffer);
""" % { "format": format, "name": operand.name })

                if operand.type == "ID":
                    inc.write("""
            st_data_t data;
            if (st_lookup(vm->symbols.id2name, %(name)s, &data)) {
                const char* s = (const char*)data;
                snprintf(buffer, sizeof(buffer), "(%%s)", s);
                YogString_append(vm, pp, buffer);
            } 
            else {
                assert(FALSE);
            }
""" % { "name": operand.name })
                elif operand.type == "offset_t":
                    inc.write("""
            snprintf(buffer, sizeof(buffer), "(%%ld)", %(name)s + pc - sizeof(inst_t) - sizeof(offset_t));
            YogString_append(vm, pp, buffer);
""" % { "name": operand.name })

            if name == "PUSH_CONST":
                inc.write("""
            YogString* str = YogValue_to_pp(vm, NULL, code->consts->ptr[index]);
            snprintf(buffer, sizeof(buffer), "(%s)", str->ptr);
            YogString_append(vm, pp, buffer);
""")

            inc.write("""
        }
        break;
""")

        inc.write(self.get_c_footer())
        self.write_file(code_inc, inc.getvalue())

    def get_types(self, yog_h):
        types = {}
        fp = open(yog_h)
        try:
            for line in fp:
                line = line.strip()
                if line.startswith("typedef "):
                    keywords = line.split()
                    type_ = keywords[-1][:-1]
                    orig = " ".join(keywords[1:-1])
                    types[type_] = orig
        finally:
            fp.close()
        return types

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

        kw = { "structs": structs.getvalue(), "macros": macros.getvalue() }
        s = self.substitute_template(inst_h_tmpl, kw)
        with open(inst_h, "w") as f:
            f.write(self.make_attention())
            f.write(s)

    def do(self, def_, opcodes_h=None, opcodes_h_tmpl=None, 
            thread_inc=None, compile_inc=None, compile_inc_tmpl=None, 
            code_inc="code.inc", yog_h="yog.h", debug=False):
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

        """
        # Generate code.inc (no templates).
        types = self.get_types(yog_h)
        self.gen_code_inc(code_inc, types)
        """

if __name__ == "__main__":
    CodeGenerator().do(argv[1])

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
