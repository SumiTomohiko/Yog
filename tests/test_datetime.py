# -*- coding: utf-8 -*-

from datetime import datetime
from testcase import TestCase

ATTBIUTES = ["year", "month", "day", "hour", "minute", "second", "microsecond"]

class TestDatetime(TestCase):

    testees = [
        datetime(1970, 1, 1, 0, 0, 0, 0),
        datetime(2038, 1, 19, 3, 14, 7, 0),
        datetime(2038, 1, 19, 3, 14, 8, 0)]
    for i, t in enumerate(testees):
        for j, name in enumerate(ATTBIUTES):
            exec """def test_property{index}(self):
    self._test(\"print(Datetime.new({year}, {month}, {day}, {hour}, {minute}, {second}, {microsecond}).{name})\", \"{expected}\")""".format(index=100 * j + i, year=t.year, month=t.month, day=t.day, hour=t.hour, minute=t.minute, second=t.second, microsecond=t.microsecond, name=name, expected=getattr(t, name))

    def test_compare0(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 0)
t1 = Datetime.new(2038, 1, 19, 3, 14, 8, 0)
print(t0 < t1)""", "true")

    def test_compare10(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 0)
t1 = Datetime.new(2038, 1, 19, 3, 14, 8, 0)
print(t0 > t1)""", "false")

    def test_compare20(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 0)
t1 = Datetime.new(2038, 1, 19, 3, 14, 8, 0)
print(t0 == t1)""", "false")

    def test_compare30(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 0)
t1 = Datetime.new(2038, 1, 19, 3, 14, 6, 0)
print(t0 < t1)""", "false")

    def test_compare40(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 0)
t1 = Datetime.new(2038, 1, 19, 3, 14, 6, 0)
print(t0 > t1)""", "true")

    def test_compare50(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 0)
t1 = Datetime.new(2038, 1, 19, 3, 14, 6, 0)
print(t0 == t1)""", "false")

    def test_compare60(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 42)
t1 = Datetime.new(2038, 1, 19, 3, 14, 7, 43)
print(t0 < t1)""", "true")

    def test_compare70(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 42)
t1 = Datetime.new(2038, 1, 19, 3, 14, 7, 43)
print(t0 > t1)""", "false")

    def test_compare80(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 42)
t1 = Datetime.new(2038, 1, 19, 3, 14, 7, 43)
print(t0 == t1)""", "false")

    def test_compare90(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 42)
t1 = Datetime.new(2038, 1, 19, 3, 14, 7, 41)
print(t0 < t1)""", "false")

    def test_compare100(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 42)
t1 = Datetime.new(2038, 1, 19, 3, 14, 7, 41)
print(t0 > t1)""", "true")

    def test_compare110(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 42)
t1 = Datetime.new(2038, 1, 19, 3, 14, 7, 41)
print(t0 == t1)""", "false")

    def test_compare120(self):
        self._test("""t0 = Datetime.new(2038, 1, 19, 3, 14, 7, 42)
t1 = Datetime.new(2038, 1, 19, 3, 14, 7, 42)
print(t0 == t1)""", "true")

    def test_compare130(self):
        self._test("print(Datetime.new() == nil)", "false")

    def test_compare140(self):
        self._test("print(Datetime.new() == 42)", "false")

    def test_to_iso8601_0(self):
        src = "print(Datetime.new(2038, 1, 19, 3, 14, 7, 42000).to_iso8601())"
        self._test(src, "2038-01-19T03:14:07,042")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
