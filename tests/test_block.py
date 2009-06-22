# -*- coding: utf-8 -*-

from testcase import TestCase

class TestBlock(TestCase):

    def test_block1(self):
        self._test("""
1.times() { [n]
    puts(n)
}
""", """0
""")

    def test_block2(self):
        self._test("""
1.times() {
    puts(42)
}
""", """42
""")

    def test_block3(self):
        self._test("""
1.times() do [n]
    puts(n)
end
""", """0
""")

    def test_block4(self):
        self._test("""
1.times() do
    puts(42)
end
""", """42
""")

    def test_block7(self):
        self._test("""
n = 42
1.times() do [n]
  puts(n)
end

puts(n)
""", """0
42
""")

    def test_block10(self):
        self._test("""
def main()
  n = 42
  1.times() do [n]
    puts(n)
  end
  puts(n)
end

main()""", """0
42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
