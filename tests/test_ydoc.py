# -*- coding: utf-8 -*-

from os import makedirs
from os.path import dirname, join
from shutil import rmtree
from tempfile import mkdtemp

from testcase import TestCase

class TestYdoc(TestCase):

    options = []

    def read(self, path):
        fp = open(path)
        try:
            return fp.read()
        finally:
            fp.close()

    def write(self, path, s):
        fp = open(path, "w")
        try:
            fp.write(s)
        finally:
            fp.close()

    def run_test(self, destdir, index):
        self._test("""
from test_ydoc import Generator
import ydoc
enable_gc_stress()

ydoc.run(\"test\", \"%(destdir)s\", \"%(index)s\", generator: Generator)
""" % { "destdir": destdir, "index": index })

    def do_test(self, src, expected=None):
        if expected is None:
            expected = src.strip()

        tmpdir = mkdtemp()
        try:
            index = join(tmpdir, "index.ydoc")
            self.write(index, src)

            destdir = join(tmpdir, "out")
            makedirs(destdir)

            self.run_test(destdir, index)

            actual = self.read(join(destdir, "index.html")).rstrip()
        finally:
            rmtree(tmpdir)

        assert expected == actual

    def test_title0(self):
        self.do_test("""
= title
""")

    def test_title10(self):
        self.do_test("""
== title
""")

    def test_list0(self):
        self.do_test("""
* foo
""")

    def test_list10(self):
        self.do_test("""
* foo
* bar
""")

    def test_list20(self):
        self.do_test("""
* foo
  * bar
""")

    def test_ordered_list0(self):
        self.do_test("""
+ foo
""")

    def test_ordered_list10(self):
        self.do_test("""
+ foo
+ bar
""")

    def test_ordered_list20(self):
        self.do_test("""
+ foo
  + bar
""")

    def test_italic0(self):
        self.do_test("_foo_")

    def test_bold0(self):
        self.do_test("*foo*")

    def test_typewriter0(self):
        self.do_test("+foo+")

    def test_class0(self):
        self.do_test("""
class: Foo
""", """class: Foo
  base:
  including:
""")

    def test_class10(self):
        self.do_test("""
class: Foo
  base: Bar
""", """class: Foo
  base: Bar
  including:
""")

    def test_class20(self):
        self.do_test("""
class: Foo
  including: Bar
""", """class: Foo
  base:
  including: Bar
""")

    def test_class30(self):
        self.do_test("""
class: Foo
  hogefugapiyo
""", """class: Foo
  base:
  including:
  hogefugapiyo
""")

    def test_class40(self):
        self.do_test("""
class: Foo
  method: bar()
""", """class: Foo
  base:
  including:
  method: bar()
    parameters:
    return:
    exceptions:
    block:
""")

    def test_class50(self):
        self.do_test("""
class: Foo
  method: bar()
  method: baz()
""", """class: Foo
  base:
  including:
  method: bar()
    parameters:
    return:
    exceptions:
    block:
  method: baz()
    parameters:
    return:
    exceptions:
    block:
""")

    def test_method0(self):
        self.do_test("""
class: Foo
  method: bar(baz)
    parameters:
      baz: quux
""", """class: Foo
  base:
  including:
  method: bar(baz)
    parameters:
      baz: quux
    return:
    exceptions:
    block:
""")

    def test_method10(self):
        self.do_test("""
class: Foo
  method: bar(baz, hoge)
    parameters:
      baz: quux
      hoge: fuga
""", """class: Foo
  base:
  including:
  method: bar(baz, hoge)
    parameters:
      baz: quux
      hoge: fuga
    return:
    exceptions:
    block:
""")

    def test_method20(self):
        self.do_test("""
class: Foo
  method: bar()
    return: hogefugapiyo
""", """class: Foo
  base:
  including:
  method: bar()
    parameters:
    return: hogefugapiyo
    exceptions:
    block:
""")

    def test_method30(self):
        self.do_test("""
class: Foo
  method: bar()
    exceptions:
      Baz: quux
""", """class: Foo
  base:
  including:
  method: bar()
    parameters:
    return:
    exceptions:
      Baz: quux
    block:
""")

    def test_method40(self):
        self.do_test("""
class: Foo
  method: bar()
    exceptions:
      Baz: quux
      Hoge: fuga
""", """class: Foo
  base:
  including:
  method: bar()
    parameters:
    return:
    exceptions:
      Baz: quux
      Hoge: fuga
    block:
""")

    def test_method50(self):
        self.do_test("""
class: Foo
  method: bar()
    block: baz(quux)
""", """class: Foo
  base:
  including:
  method: bar()
    parameters:
    return:
    exceptions:
    block: baz(quux)
""")

    def test_property0(self):
        self.do_test("""
class: Foo
  property: bar
""", """class: Foo
  base:
  including:
  property: bar
    type:
""")

    def test_property10(self):
        self.do_test("""
class: Foo
  property: bar
    type: Baz
""", """class: Foo
  base:
  including:
  property: bar
    type: Baz
""")

    def test_property20(self):
        self.do_test("""
class: Foo
  property: bar
    type: Baz
    quux
""", """class: Foo
  base:
  including:
  property: bar
    type: Baz
    quux
""")

    def test_attribute0(self):
        self.do_test("""
class: Foo
  attribute: bar
""", """class: Foo
  base:
  including:
  attribute: bar
    type:
""")

    def test_attribute10(self):
        self.do_test("""
class: Foo
  attribute: bar
    type: Baz
""", """class: Foo
  base:
  including:
  attribute: bar
    type: Baz
""")

    def test_attribute20(self):
        self.do_test("""
class: Foo
  attribute: bar
    type: Baz
    quux
""", """class: Foo
  base:
  including:
  attribute: bar
    type: Baz
    quux
""")

    def test_classmethod0(self):
        self.do_test("""
class: Foo
  classmethod: bar(baz)
    parameters:
      baz: quux
""", """class: Foo
  base:
  including:
  classmethod: bar(baz)
    parameters:
      baz: quux
    return:
    exceptions:
    block:
""")

    def test_classmethod10(self):
        self.do_test("""
class: Foo
  classmethod: bar(baz, hoge)
    parameters:
      baz: quux
      hoge: fuga
""", """class: Foo
  base:
  including:
  classmethod: bar(baz, hoge)
    parameters:
      baz: quux
      hoge: fuga
    return:
    exceptions:
    block:
""")

    def test_classmethod20(self):
        self.do_test("""
class: Foo
  classmethod: bar()
    return: hogefugapiyo
""", """class: Foo
  base:
  including:
  classmethod: bar()
    parameters:
    return: hogefugapiyo
    exceptions:
    block:
""")

    def test_classmethod30(self):
        self.do_test("""
class: Foo
  classmethod: bar()
    exceptions:
      Baz: quux
""", """class: Foo
  base:
  including:
  classmethod: bar()
    parameters:
    return:
    exceptions:
      Baz: quux
    block:
""")

    def test_classmethod40(self):
        self.do_test("""
class: Foo
  classmethod: bar()
    exceptions:
      Baz: quux
      Hoge: fuga
""", """class: Foo
  base:
  including:
  classmethod: bar()
    parameters:
    return:
    exceptions:
      Baz: quux
      Hoge: fuga
    block:
""")

    def test_classmethod50(self):
        self.do_test("""
class: Foo
  classmethod: bar()
    block: baz(quux)
""", """class: Foo
  base:
  including:
  classmethod: bar()
    parameters:
    return:
    exceptions:
    block: baz(quux)
""")

    def test_function0(self):
        self.do_test("""
function: bar()
""", """function: bar()
  parameters:
  return:
  exceptions:
  block:
""")

    def test_function5(self):
        self.do_test("""
function: bar(baz)
  parameters:
    baz: quux
""", """function: bar(baz)
  parameters:
    baz: quux
  return:
  exceptions:
  block:
""")

    def test_function10(self):
        self.do_test("""
function: bar(baz, hoge)
  parameters:
    baz: quux
    hoge: fuga
""", """function: bar(baz, hoge)
  parameters:
    baz: quux
    hoge: fuga
  return:
  exceptions:
  block:
""")

    def test_function20(self):
        self.do_test("""
function: bar()
  return: hogefugapiyo
""", """function: bar()
  parameters:
  return: hogefugapiyo
  exceptions:
  block:
""")

    def test_function30(self):
        self.do_test("""
function: bar()
  exceptions:
    Baz: quux
""", """function: bar()
  parameters:
  return:
  exceptions:
    Baz: quux
  block:
""")

    def test_function40(self):
        self.do_test("""
function: bar()
  exceptions:
    Baz: quux
    Hoge: fuga
""", """function: bar()
  parameters:
  return:
  exceptions:
    Baz: quux
    Hoge: fuga
  block:
""")

    def test_function50(self):
        self.do_test("""
function: bar()
  block: baz(quux)
""", """function: bar()
  parameters:
  return:
  exceptions:
  block: baz(quux)
""")

    def test_data0(self):
        self.do_test("""
data: foo
  type: Bar
""")

    def test_data10(self):
        self.do_test("""
data: foo
  type: Bar
  baz
""")

    def do_test2(self, srcs, index, expecteds):
        tmpdir = mkdtemp()
        try:
            for path, src in srcs:
                dir = join(tmpdir, dirname(path))
                makedirs(dir)
                self.write(join(tmpdir, path), src)

            destdir = join(tmpdir, "out")
            makedirs(destdir)

            self.run_test(destdir, index)

            for path, expected in expecteds:
                actual = self.read(join(destdir, path))
                assert expected == actual
        finally:
            rmtree(tmpdir)

    def test_link0(self):
        srcs = { "index.ydoc": "[index2.ydoc]", "index2.ydoc": "" }
        expecteds = { "index.html": "[index2.html]" }
        self.do_test2(srcs, "index.ydoc", expecteds)

    def test_link10(self):
        srcs = {
            "index.ydoc": "[foo/index2.ydoc]",
            join("foo", "index2.ydoc"): "" }
        expecteds = { "index.html": "[foo/index2.html]" }
        self.do_test2(srcs, "index.ydoc", expecteds)

    def test_link20(self):
        srcs = {
            "index.ydoc": "[foo/bar/index2.ydoc]",
            join("foo", "bar", "index2.ydoc"): "" }
        expecteds = { "index.html": "[foo/bar/index2.html]" }
        self.do_test2(srcs, "index.ydoc", expecteds)

    def test_link30(self):
        srcs = {
            "index.ydoc": "[foo/index2.ydoc]",
            join("foo", "index2.ydoc"): "[bar/index3.ydoc]",
            join("foo", "bar", "index3.ydoc"): "" }
        expecteds = { join("foo", "index2.html"): "[bar/index3.html]" }
        self.do_test2(srcs, "index.ydoc", expecteds)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
