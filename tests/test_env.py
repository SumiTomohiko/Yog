# -*- coding: utf-8 -*-

from os import environ
from testcase import TestCase

class TestEnv(TestCase):

    def delete_env(self, key):
        try:
            del environ[key]
        except KeyError:
            pass

    def test_subscript0(self):
        environ["foo"] = "bar"
        self._test("print(ENV[\"foo\"])", "bar")

    def test_subscript10(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("KeyError: foo")
        self.delete_env("foo")
        self._test("print(ENV[\"foo\"])", stderr=test_stderr)

    def test_subscript_assign0(self):
        self._test("""
ENV[\"foo\"] = \"bar\"
print(ENV[\"foo\"])
""", "bar")

    def test_get0(self):
        self.delete_env("foo")
        self._test("print(ENV.get(\"foo\"))", "nil")

    def test_get10(self):
        self.delete_env("foo")
        self._test("print(ENV.get(\"foo\", 42))", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
