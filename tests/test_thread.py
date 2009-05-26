# -*- coding: utf-8 -*-

from tests import TestCase

class TestThread(TestCase):

    def test_thread1(self):
        self._test("""
def main()
  thread = Thread.new() do
    puts 42
  end
  thread.run()
  thread.join()
end

main()""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
