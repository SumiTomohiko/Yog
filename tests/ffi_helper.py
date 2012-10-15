# -*- coding: utf-8 -*-

from os.path import dirname, join
from testcase import TestCase

class Base(TestCase):

    def do_gcc_test(self, name, type, width, val):
        so = join(dirname(__file__), name + ".so")
        self._test("""Foo = StructClass.new(\"Foo\")
Foo.define_fields([[[\'{type}, {width}], \'bar]])
foo = Foo.new()
foo.bar = {val}
lib = load_lib(\"{so}\")
f = lib.load_func(\"{name}\", [[\'pointer, Foo]], \'void)
f(foo)""".format(**locals()), str(val))

    def do_ng_test(self, type, width, min, val, msg):
        fmt = "ValueError: Value must be {msg} or equal {min}, not {val}"
        msg = fmt.format(**locals())
        def test_stderr(stderr):
            assert 0 < stderr.find(msg)
        self._test("""Foo = StructClass.new(\"Foo\")
Foo.define_fields([[[\'{type}, {width}], \'bar]])
foo = Foo.new()
foo.bar = {val}""".format(**locals()), stderr=test_stderr)

    def do_ok_test(self, type, width, val):
        self._test("""Foo = StructClass.new(\"Foo\")
Foo.define_fields([[[\'{type}, {width}], \'bar]])
foo = Foo.new()
foo.bar = {val}
print(foo.bar)""".format(**locals()), str(val))

def define_range_test(klass, type, width, min, max):
    name = "{type}_{width}".format(**locals())
    src = """def test_bit_field_{name}_under_min(self):
    self.do_ng_test(\"{type}\", {width}, {min}, {min} - 1, \"greater\")

def test_bit_field_{name}_min(self):
    self.do_ok_test(\"{type}\", {width}, {min})

def test_bit_field_{name}_min_gcc(self):
    self.do_gcc_test(\"test_{name}\", \"{type}\", {width}, {min})

def test_bit_field_{name}_max(self):
    self.do_ok_test(\"{type}\", {width}, {max})

def test_bit_field_{name}_max_gcc(self):
    self.do_gcc_test(\"test_{name}\", \"{type}\", {width}, {max})

def test_bit_field_{name}_over_max(self):
    self.do_ng_test(\"{type}\", {width}, {max}, {max} + 1, \"less\")""".format(**locals())
    d = {}
    exec(src, d)
    for key, val in d.items():
        setattr(klass, key, val)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
