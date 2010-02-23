# -*- coding: utf-8 -*-

from testcase import TestCase

class TestKlass(TestCase):

    def test_AttributeError0(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("AttributeError: Class object has no attribute \"foo\"")

        self._test("""
42.class.foo
""", stderr=test_stderr)

    def test_method(self):
        self._test("""
class Foo

    def bar()
        puts(42)
    end
end

foo = Foo.new()
foo.bar()
""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
