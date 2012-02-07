# -*- coding: utf-8 -*-

from os import name
import pytest
from testcase import TestCase

@pytest.mark.skipif("name == \"nt\"")
class TestThread(TestCase):

    def test_thread1(self):
        self._test("""
import concurrent

def main()
  thread = concurrent.Thread.new() do
    puts(42)
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
    puts(42)
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
  thread_count = 32
  int = concurrent.AtomicInt.new(0)
  barrier = concurrent.Barrier.new(thread_count)
  threads = []

  i = 0
  while i < thread_count
    thread = concurrent.Thread.new() do
      barrier.wait!()
      4096.times() do
          int.inc!()
      end
    end
    thread.run()
    threads << thread

    i = i + 1
  end

  threads.each() do |thread|
    thread.join()
  end

  puts(int.get())
end

main()
""", """131072
""", timeout=1200)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
