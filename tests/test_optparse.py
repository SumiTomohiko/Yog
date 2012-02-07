# -*- coding: utf-8 -*-

from testcase import TestCase

class TestOptparse(TestCase):

    options = []

    def test_import0(self):
        self._test("""
import optparse
enable_gc_stress()
""", options=["--gc-stress"])

    def test_switch0(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_switch(\"x\", \"foo\") do
  print(42)
end
opt.parse([\"-x\"])
""", "42")

    def test_switch10(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_switch(\"x\", \"foo\") do
  print(42)
end
opt.parse([\"-x\", \"-x\"])
""", "4242")

    def test_switch20(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_switch(\"x\", \"foo\") do
  print(42)
end
opt.parse([\"--foo\"])
""", "42")

    def test_switch30(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_switch(\"x\", \"foo\") do
  print(42)
end
opt.parse([\"--foo\", \"--foo\"])
""", "4242")

    def test_switch40(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_switch(\"x\", \"foo\") do
  print(42)
end
opt.parse([])
""", "")

    def test_switch50(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_switch(\"x\", \"foo\") do
  print(42)
end
opt.on_switch(\"y\", \"bar\") do
  print(26)
end
opt.parse([\"-xy\"])
""", "4226")

    def test_option0(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_option(\"x\", \"foo\") do |val|
  print(val)
end
opt.parse([\"-x\", \"42\"])
""", "42")

    def test_option10(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_option(\"x\", \"foo\") do |val|
  print(val)
end
opt.parse([\"-x42\"])
""", "42")

    def test_option20(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_option(\"x\", \"foo\") do |val|
  print(val)
end
opt.parse([\"--foo\", \"42\"])
""", "42")

    def test_option30(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_option(\"x\", \"foo\") do |val|
  print(val)
end
opt.parse([\"--foo=42\"])
""", "42")

    def test_return0(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_switch(\"x\", \"foo\") do
end
print(opt.parse([\"-x\", \"42\"]))
""", "[\"42\"]")

    def test_return10(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_switch(\"x\", \"foo\") do
end
print(opt.parse([\"42\"]))
""", "[\"42\"]")

    def test_return20(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_switch(\"x\", \"foo\") do
end
print(opt.parse([\"--\", \"42\"]))
""", "[\"42\"]")

    def test_return30(self):
        self._test("""
import optparse
enable_gc_stress()
opt = optparse.OptionParser.new()
opt.on_option(\"x\", \"foo\") do
end
print(opt.parse([\"--foo\", \"42\", \"26\"]))
""", "[\"26\"]")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
