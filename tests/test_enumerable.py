# -*- coding: utf-8 -*-

from re import sub
from testcase import TestCase, enumerate_tuples

def make_test_name(src):
    return sub(r"[^\w]", "_", src)

class TestEnumerable(TestCase):

    for src, expected in (
            ("[].take(0)", "[]"),
            ("[42].take(0)", "[]"),
            ("[42].take(1)", "[42]"),
            ("[42].take(2)", "[42]"),
            ("[42, 26].take(1)", "[42]"),
            ("[42, 26].take(2)", "[42, 26]"),
            ("[].drop(0)", "[]"),
            ("[42].drop(0)", "[42]"),
            ("[42].drop(1)", "[]"),
            ("[42].drop(2)", "[]"),
            ("[42, 26].drop(1)", "[26]"),
            ("[42, 26].drop(2)", "[]")):
        exec("""def test_{name}(self):
    self._test(\"print({src})\", \"{expected}\")
""".format(src=src, expected=expected, name=make_test_name(src)))

    for i, a, expr, expected in enumerate_tuples((
            ("[]", "true", "[]"),
            ("[]", "false", "[]"),
            ("[42]", "true", "[42]"),
            ("[42]", "false", "[]"),
            ("[42, 26]", "false", "[]"),
            ("[42, 26]", "elem == 42", "[42]"),
            ("[42, 26]", "true", "[42, 26]"))):
        exec("""def test_take_while{i}(self):
    self._test(\"\"\"print({a}.take_while() do |elem|
  next {expr}
end)\"\"\", \"{expected}\")
""".format(i=10 * i, a=a, expr=expr, expected=expected))

    for i, a, expr, expected in enumerate_tuples((
            ("[]", "true", "[]"),
            ("[]", "false", "[]"),
            ("[42]", "true", "[]"),
            ("[42]", "false", "[42]"),
            ("[42, 26]", "true", "[]"),
            ("[42, 26]", "elem == 26", "[42]"),
            ("[42, 26]", "false", "[42, 26]"))):
        exec("""def test_take_until{i}(self):
    self._test(\"\"\"print({a}.take_until() do |elem|
  next {expr}
end)\"\"\", \"{expected}\")
""".format(i=10 * i, a=a, expr=expr, expected=expected))

    for i, a, expr, expected in enumerate_tuples((
            ("[]", "true", "[]"),
            ("[]", "false", "[]"),
            ("[42]", "true", "[]"),
            ("[42]", "false", "[42]"),
            ("[42, 26]", "true", "[]"),
            ("[42, 26]", "elem == 42", "[26]"),
            ("[42, 26]", "false", "[42, 26]"))):
        exec("""def test_drop_while{i}(self):
    self._test(\"\"\"print({a}.drop_while() do |elem|
  next {expr}
end)\"\"\", \"{expected}\")
""".format(i=10 * i, a=a, expr=expr, expected=expected))

    for i, a, expr, expected in enumerate_tuples((
            ("[]", "true", "[]"),
            ("[]", "false", "[]"),
            ("[42]", "true", "[42]"),
            ("[42]", "false", "[]"),
            ("[42, 26]", "false", "[]"),
            ("[42, 26]", "elem == 26", "[26]"),
            ("[42, 26]", "true", "[42, 26]"))):
        exec("""def test_drop_until{i}(self):
    self._test(\"\"\"print({a}.drop_until() do |elem|
  next {expr}
end)\"\"\", \"{expected}\")
""".format(i=10 * i, a=a, expr=expr, expected=expected))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=python
