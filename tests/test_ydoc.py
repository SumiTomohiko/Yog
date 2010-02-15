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
#enable_gc_stress()

ydoc.run(\"test\", \"%(destdir)s\", \"%(index)s\", generator: Generator)
""" % { "destdir": destdir, "index": index })

    def do_test(self, src, expected):
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

        e = expected.rstrip().split("\n")
        a = actual.rstrip().split("\n")
        assert len(e) == len(a)
        for i in range(len(e)):
            assert e[i].rstrip() == a[i].rstrip()

    def test_pretty0(self):
        self.do_test("""
foo

  [42, 26]
""", """<p>foo</p>
<pre>[42, 26]
</pre>
""")

    def test_title0(self):
        self.do_test("""
= title
""", """<h1>title</h1>
""")

    def test_title10(self):
        self.do_test("""
== title
""", """<h2>title</h2>
""")

    def test_list0(self):
        self.do_test("""
* foo
""", """<ul>
<li>foo</li>
</ul>
""")

    def test_list10(self):
        self.do_test("""
* foo
* bar
""", """<ul>
<li>foo</li>
<li>bar</li>
</ul>
""")

    def test_list20(self):
        self.do_test("""
* foo
  * bar
""", """<ul>
<li>foo<ul>
<li>bar</li>
</ul>
</li>
</ul>
""")

    def test_ordered_list0(self):
        self.do_test("""
+ foo
""", """<ol>
<li>foo</li>
</ol>
""")

    def test_ordered_list10(self):
        self.do_test("""
+ foo
+ bar
""", """<ol>
<li>foo</li>
<li>bar</li>
</ol>
""")

    def test_ordered_list20(self):
        self.do_test("""
+ foo
  + bar
""", """<ol>
<li>foo<ol>
<li>bar</li>
</ol>
</li>
</ol>
""")

    def test_italic0(self):
        self.do_test("_foo_", "<p><i>foo</i></p>\n")

    def test_italic10(self):
        self.do_test("_\\__", "<p><i>_</i></p>\n")

    def test_bold0(self):
        self.do_test("*foo*", "<p><em>foo</em></p>\n")

    def test_bold10(self):
        self.do_test("*\\**", "<p><em>*</em></p>\n")

    def test_typewriter0(self):
        self.do_test("+foo+", "<p><tt>foo</tt></p>\n")

    def test_typewriter10(self):
        self.do_test("+\\++", "<p><tt>+</tt></p>\n")

    def test_class0(self):
        self.do_test("""
class: Foo
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
</class>
""")

    def test_class10(self):
        self.do_test("""
class: Foo
  base: Bar
""", """<class>
<name>Foo</name>
<base>Bar</base>
<including></including>
</class>
""")

    def test_class20(self):
        self.do_test("""
class: Foo
  including: Bar
""", """<class>
<name>Foo</name>
<base></base>
<including>Bar</including>
</class>
""")

    def test_class30(self):
        self.do_test("""
class: Foo
  hogefugapiyo
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<p>hogefugapiyo</p>
</class>
""")

    def test_class40(self):
        self.do_test("""
class: Foo
  method: bar()
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<method>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</method>
</class>
""")

    def test_class50(self):
        self.do_test("""
class: Foo
  method: bar()
  method: baz()
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<method>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</method>
<method>
<signature>baz()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</method>
</class>
""")

    def test_method0(self):
        self.do_test("""
class: Foo
  method: bar(baz)
    parameters:
      baz: quux
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<method>
<signature>bar(baz)</signature>
<parameters>
<parameter>
<name>baz</name>
<description>quux</description>
</parameter>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</method>
</class>
""")

    def test_method10(self):
        self.do_test("""
class: Foo
  method: bar(baz, hoge)
    parameters:
      baz: quux
      hoge: fuga
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<method>
<signature>bar(baz, hoge)</signature>
<parameters>
<parameter>
<name>baz</name>
<description>quux</description>
</parameter>
<parameter>
<name>hoge</name>
<description>fuga</description>
</parameter>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</method>
</class>
""")

    def test_method20(self):
        self.do_test("""
class: Foo
  method: bar()
    return: hogefugapiyo
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<method>
<signature>bar()</signature>
<parameters>
</parameters>
<return>hogefugapiyo</return>
<exceptions>
</exceptions>
<block></block>
</method>
</class>
""")

    def test_method30(self):
        self.do_test("""
class: Foo
  method: bar()
    exceptions:
      Baz: quux
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<method>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
<exception>
<type>Baz</type>
<description>quux</description>
</exception>
</exceptions>
<block></block>
</method>
</class>
""")

    def test_method40(self):
        self.do_test("""
class: Foo
  method: bar()
    exceptions:
      Baz: quux
      Hoge: fuga
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<method>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
<exception>
<type>Baz</type>
<description>quux</description>
</exception>
<exception>
<type>Hoge</type>
<description>fuga</description>
</exception>
</exceptions>
<block></block>
</method>
</class>
""")

    def test_method50(self):
        self.do_test("""
class: Foo
  method: bar()
    block: baz(quux)
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<method>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block>baz(quux)</block>
</method>
</class>
""")

    def test_property0(self):
        self.do_test("""
class: Foo
  property: bar
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<property>
<name>bar</name>
<type></type>
</property>
</class>
""")

    def test_property10(self):
        self.do_test("""
class: Foo
  property: bar
    type: Baz
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<property>
<name>bar</name>
<type>Baz</type>
</property>
</class>
""")

    def test_property20(self):
        self.do_test("""
class: Foo
  property: bar
    type: Baz
    quux
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<property>
<name>bar</name>
<type>Baz</type>
<p>quux</p>
</property>
</class>
""")

    def test_attribute0(self):
        self.do_test("""
class: Foo
  attribute: bar
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<attribute>
<name>bar</name>
<type></type>
</attribute>
</class>
""")

    def test_attribute10(self):
        self.do_test("""
class: Foo
  attribute: bar
    type: Baz
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<attribute>
<name>bar</name>
<type>Baz</type>
</attribute>
</class>
""")

    def test_attribute20(self):
        self.do_test("""
class: Foo
  attribute: bar
    type: Baz
    quux
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<attribute>
<name>bar</name>
<type>Baz</type>
<p>quux</p>
</attribute>
</class>
""")

    def test_classmethod0(self):
        self.do_test("""
class: Foo
  classmethod: bar(baz)
    parameters:
      baz: quux
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<classmethod>
<signature>bar(baz)</signature>
<parameters>
<parameter>
<name>baz</name>
<description>quux</description>
</parameter>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</classmethod>
</class>
""")

    def test_classmethod10(self):
        self.do_test("""
class: Foo
  classmethod: bar(baz, hoge)
    parameters:
      baz: quux
      hoge: fuga
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<classmethod>
<signature>bar(baz, hoge)</signature>
<parameters>
<parameter>
<name>baz</name>
<description>quux</description>
</parameter>
<parameter>
<name>hoge</name>
<description>fuga</description>
</parameter>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</classmethod>
</class>
""")

    def test_classmethod20(self):
        self.do_test("""
class: Foo
  classmethod: bar()
    return: hogefugapiyo
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<classmethod>
<signature>bar()</signature>
<parameters>
</parameters>
<return>hogefugapiyo</return>
<exceptions>
</exceptions>
<block></block>
</classmethod>
</class>
""")

    def test_classmethod30(self):
        self.do_test("""
class: Foo
  classmethod: bar()
    exceptions:
      Baz: quux
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<classmethod>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
<exception>
<type>Baz</type>
<description>quux</description>
</exception>
</exceptions>
<block></block>
</classmethod>
</class>
""")

    def test_classmethod40(self):
        self.do_test("""
class: Foo
  classmethod: bar()
    exceptions:
      Baz: quux
      Hoge: fuga
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<classmethod>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
<exception>
<type>Baz</type>
<description>quux</description>
</exception>
<exception>
<type>Hoge</type>
<description>fuga</description>
</exception>
</exceptions>
<block></block>
</classmethod>
</class>
""")

    def test_classmethod50(self):
        self.do_test("""
class: Foo
  classmethod: bar()
    block: baz(quux)
""", """<class>
<name>Foo</name>
<base></base>
<including></including>
<classmethod>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block>baz(quux)</block>
</classmethod>
</class>
""")

    def test_function0(self):
        self.do_test("""
function: bar()
""", """<function>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</function>
""")

    def test_function5(self):
        self.do_test("""
function: bar(baz)
  parameters:
    baz: quux
""", """<function>
<signature>bar(baz)</signature>
<parameters>
<parameter>
<name>baz</name>
<description>quux</description>
</parameter>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</function>
""")

    def test_function10(self):
        self.do_test("""
function: bar(baz, hoge)
  parameters:
    baz: quux
    hoge: fuga
""", """<function>
<signature>bar(baz, hoge)</signature>
<parameters>
<parameter>
<name>baz</name>
<description>quux</description>
</parameter>
<parameter>
<name>hoge</name>
<description>fuga</description>
</parameter>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block></block>
</function>
""")

    def test_function20(self):
        self.do_test("""
function: bar()
  return: hogefugapiyo
""", """<function>
<signature>bar()</signature>
<parameters>
</parameters>
<return>hogefugapiyo</return>
<exceptions>
</exceptions>
<block></block>
</function>
""")

    def test_function30(self):
        self.do_test("""
function: bar()
  exceptions:
    Baz: quux
""", """<function>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
<exception>
<type>Baz</type>
<description>quux</description>
</exception>
</exceptions>
<block></block>
</function>
""")

    def test_function40(self):
        self.do_test("""
function: bar()
  exceptions:
    Baz: quux
    Hoge: fuga
""", """<function>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
<exception>
<type>Baz</type>
<description>quux</description>
</exception>
<exception>
<type>Hoge</type>
<description>fuga</description>
</exception>
</exceptions>
<block></block>
</function>
""")

    def test_function50(self):
        self.do_test("""
function: bar()
  block: baz(quux)
""", """<function>
<signature>bar()</signature>
<parameters>
</parameters>
<return></return>
<exceptions>
</exceptions>
<block>baz(quux)</block>
</function>
""")

    def test_data0(self):
        self.do_test("""
data: foo
  type: Bar
""", """<data>
<name>foo</name>
<type>Bar</type>
</data>
""")

    def test_data10(self):
        self.do_test("""
data: foo
  type: Bar
  baz
""", """<data>
<name>foo</name>
<type>Bar</type>
<p>baz</p>
</data>
""")

    def test_paragraph0(self):
        self.do_test("""foo

bar
""", """<p>foo</p>
<p>bar</p>
""")

    def do_test2(self, srcs, index, expecteds):
        tmpdir = mkdtemp()
        try:
            for path in srcs:
                dir = join(tmpdir, dirname(path))
                try:
                    makedirs(dir)
                except OSError:
                    pass
                self.write(join(tmpdir, path), srcs[path])

            destdir = join(tmpdir, "out")
            makedirs(destdir)

            self.run_test(destdir, join(tmpdir, index))

            for path in expecteds:
                actual = self.read(join(destdir, path))
                assert expecteds[path].rstrip() == actual.rstrip()
        finally:
            rmtree(tmpdir)

    def test_link0(self):
        srcs = { "index.ydoc": "[index2.ydoc]", "index2.ydoc": "" }
        expecteds = { "index.html": "<p>{}[index2.html]</p>" }
        self.do_test2(srcs, "index.ydoc", expecteds)

    def test_link10(self):
        srcs = {
            "index.ydoc": "[foo/index2.ydoc]",
            join("foo", "index2.ydoc"): "" }
        expecteds = { "index.html": "<p>{}[foo/index2.html]</p>" }
        self.do_test2(srcs, "index.ydoc", expecteds)

    def test_link20(self):
        srcs = {
            "index.ydoc": "[foo/bar/index2.ydoc]",
            join("foo", "bar", "index2.ydoc"): "" }
        expecteds = { "index.html": "<p>{}[foo/bar/index2.html]</p>" }
        self.do_test2(srcs, "index.ydoc", expecteds)

    def test_link30(self):
        srcs = {
            "index.ydoc": "[foo/index2.ydoc]",
            join("foo", "index2.ydoc"): "[bar/index3.ydoc]",
            join("foo", "bar", "index3.ydoc"): "" }
        expecteds = { join("foo", "index2.html"): "<p>{}[bar/index3.html]</p>" }
        self.do_test2(srcs, "index.ydoc", expecteds)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
