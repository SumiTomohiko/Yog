#! python
# -*- coding: utf-8 -*-

from glob import glob
import re

class DeclarationInserter(object):

    start = "/* PROTOTYPE_START */"
    end = "/* PROTOTYPE_END */"
    files = { 
            "include/yog/module.h": [ "src/module.c", ], 
            "include/yog/file.h": [ "src/file.c", ], 
            "include/yog/dict.h": [ "src/dict.c", ], 
            "include/yog/classmethod.h": [ "src/classmethod.c", ], 
            "include/yog/property.h": [ "src/property.c", ], 
            "include/yog/symbol.h": [ "src/symbol.c", ], 
            "include/yog/repl.h": [ "src/repl.c", ], 
            "include/yog/yog.h": [ "src/value.c", ], 
            "include/yog/builtins.h": [ "src/builtins.c", ], 
            "include/yog/error.h": [ "src/error.c", ], 
            "include/yog/bool.h": [ "src/bool.c", ], 
            "include/yog/fixnum.h": [ "src/fixnum.c", ], 
            "include/yog/arg.h": [ "src/arg.c", ], 
            "include/yog/block.h": [ "src/block.c", ], 
            "include/yog/klass.h": [ "src/klass.c", ], 
            "include/yog/nil.h": [ "src/nil.c", ], 
            "include/yog/function.h": [ "src/function.c", ], 
            "include/yog/binary.h": [ "src/binary.c", ], 
            "include/yog/package.h": [ "src/package.c", ], 
            "include/yog/code.h": [ "src/code.c", "src/code.inc", ], 
            "include/yog/compile.h": [ "src/compile.c", ], 
            "include/yog/array.h": [ "src/array.c", ], 
            "include/yog/parser.h": [
                    "src/lexer.c", "src/parser.y", 
                    ], 
            "include/yog/regexp.h": [ "src/regexp.c", ], 
            "include/yog/string.h": [ "src/string.c", ], 
            "include/yog/encoding.h": [ "src/encoding.c", ], 
            "include/yog/exception.h": [ 
                    "src/exception.c", "src/stacktrace.c", ], 
            "include/yog/inst.h.tmpl": [ "src/inst.c", ], 
            "include/yog/table.h": [ "src/table.c", ], 
            "include/yog/gc/mark-sweep-compact.h": [ 
                "src/gc/mark-sweep-compact.c", ], 
            "include/yog/gc/copying.h": [ "src/gc/copying.c", ], 
            "include/yog/gc/mark-sweep.h": [ "src/gc/mark-sweep.c", ], 
            "include/yog/gc/bdw.h": [ "src/gc/bdw.c", ], 
            "include/yog/gc/generational.h": [ "src/gc/generational.c", ], 
            "include/yog/thread.h": [ "src/thread.c", ], 
            "include/yog/vm.h": [ "src/vm.c", ], 
            "include/yog/object.h": [ "src/object.c", ], 
            "include/yog/frame.h": [ "src/frame.c", ], 
            "include/yog/float.h": [ "src/float.c", ], 
            "include/yog/eval.h": [ "src/eval.c", ], 
            "include/yog/gc.h": [ "src/gc.c", ], 
            "include/yog/bignum.h": [ "src/bignum.c", ], 
            }

    def _find(self, lines, s, start):
        processed = []

        i = start
        while True:
            try:
                line = lines[i]
            except IndexError:
                break
            try:
                line.index(s)
            except ValueError:
                processed.append(line)
            else:
                processed.append(line)
                break

            i += 1

        return processed, i

    def _rewrite_header(self, header_filename, declarations):
        try:
            fp = open(header_filename)
            try:
                lines = fp.readlines()
            finally:
                fp.close()
        except IOError:
            lines = []

        header, i = self._find(lines, self.start, 0)
        header.append("""
/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

""")

        for filename in sorted(declarations):
            functions = declarations[filename]
            header.append("/* %(filename)s */\n" % { "filename": filename })
            for function in functions:
                header.append(function + "\n")
            header.append("\n")

        old, i = self._find(lines, self.end, i)
        header.extend(lines[i:])

        fp = open(header_filename, "w")
        try:
            fp.write("".join(header))
        finally:
            fp.close()

    re_type = re.compile(r"\A(?:(?:struct|unsigned|const)\s+)?\w+\s*\**\Z")
    re_function = re.compile(r"\A(?P<name>\w+)\s*\(.*\)\Z")
    re_function_pointer \
            = re.compile(r"\A(?P<head>[\w\*]+\s+\(\*)\w+(?P<tail>\)\(.*\))\Z")

    def _split_params(self, line):
        params = []
        paren_depth = 0
        param = []
        for c in line:
            if (paren_depth == 0) and (c == ","):
                params.append("".join(param).strip())
                param = []
            else:
                if (c == "("):
                    paren_depth += 1
                elif (c == ")"):
                    paren_depth -= 1
                param.append(c)
        last_param = "".join(param).strip()
        if last_param != "":
            params.append(last_param)

        return params

    def _get_functions(self, filename):
        declarations = {}

        lines = []
        fp = open(filename)
        try:
            lines = fp.readlines()
        finally:
            fp.close()

        while True:
            try:
                line = lines.pop(0).strip()
            except IndexError:
                break
            if self.re_type.search(line):
                return_type = line

                while True:
                    try:
                        line = lines.pop(0).strip()
                    except IndexError:
                        break
                    else:
                        if line != "":
                            break

                m = self.re_function.search(line)
                if m:
                    name = m.group("name")

                    args = []
                    n = line.index("(")
                    m = line.rindex(")")
                    params = self._split_params(line[n + 1:m])
                    for param in params:
                        param = param.strip()
                        if param == "...":
                            args.append(param)
                        else:
                            m = self.re_function_pointer.search(param)
                            if m:
                                type_ = m.group("head") + m.group("tail")
                            else:
                                param = param.strip()
                                param = param.split(" ")
                                try:
                                    n = param[-1].rindex("*")
                                except ValueError:
                                    type_ = " ".join(param[:-1])
                                else:
                                    type_ = " ".join(param[:-1]) \
                                            + param[-1][:n + 1]
                            args.append(type_)

                    declarations[name] = \
                            "%(return_type)s %(name)s(%(args)s);" % { 
                                    "return_type": return_type, "name": name, 
                                    "args": ", ".join(args) }

        retval = []
        for name in sorted(declarations):
            retval.append(declarations[name])

        return retval

    def do(self):
        for header, sources in self.files.items():
            declarations = {}
            for source in sources:
                declarations[source] = self._get_functions(source)
            self._rewrite_header(header, declarations)

if __name__ == "__main__":
    inserter = DeclarationInserter()
    inserter.do()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
