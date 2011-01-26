# -*- coding: utf-8 -*-

from testcase import TestCase

class TestProcess(TestCase):

    def test_run0(self):
        self._test("""
proc = Process.new([\"/bin/echo\"])
proc.run()
print(proc.wait())
""", "0")

    def test_poll0(self):
        self._test("""
proc = Process.new([\"/bin/sleep\", \"3\"])
proc.run()
while (status = proc.poll()) == nil
end
print(status)
""", "0")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
