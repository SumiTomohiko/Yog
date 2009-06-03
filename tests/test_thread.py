# -*- coding: utf-8 -*-

from testcase import TestCase

class TestThread(TestCase):

    def test_thread1(self):
        self._test("""
import concurrent

def main()
  thread = concurrent.Thread.new() do
    puts 42
  end
  thread.run()
end

main()""", """42
""")

    def test_join1(self):
        self._test("""
import concurrent

def main()
  thread = concurrent.Thread.new() do
    puts 42
  end
  thread.run()
  thread.join()
end

main()""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
