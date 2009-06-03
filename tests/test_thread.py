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

    def test_AtomicInt1(self):
        self._test("""
import concurrent

def main()
  int = concurrent.AtomicInt.new()
  barrier = concurrent.Barrier.new()

  i = 0
  while i < 32
    thread = concurrent.Thread.new() do
      barrier.wait()

      j = 0
      while j < 2048
        int.inc!()
        j += 1
      end
    end
    thread.start()

    i += 1
  end

  puts int.get()
end

main()""", """65536
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
