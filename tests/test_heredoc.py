# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestHereDoc(TestCase):

    def test_one_heredoc0(self):
        self._test("""
print(<<EOF)
42
EOF
""", """42
""")

    def test_two_heredoc0(self):
        self._test("""
print(<<EOF1, <<EOF2)
42
EOF1
26
EOF2
""", """42
26
""")

    def test_heredoc_position0(self):
        self._test("""
print(<<EOF
42
EOF
)
""", """42
""")

    def test_error0(self):
        def test_stderr(stderr):
            m = match("SyntaxError: file \"[^\"]+\", line 2: EOF while scanning heredoc\n", stderr)
            assert m is not None

        self._test("""
print(<<EOF)
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
