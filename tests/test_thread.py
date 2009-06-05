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
  thread_count = 256
  int = concurrent.AtomicInt.new(0)
  barrier = concurrent.Barrier.new(thread_count)

  i = 0
  while i < thread_count
    thread = concurrent.Thread.new() do
      barrier.wait!()
      int.inc!()
    end
    thread.run()

    i = i + 1
  end

  puts int.get()
end

main()""", """256
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
