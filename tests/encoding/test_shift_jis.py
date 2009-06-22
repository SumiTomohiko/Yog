# -*- coding: utf-8 -*-

from encoding import TestEncoding

class TestShiftJis(TestEncoding):

    encoding = "Shift-JIS"

    def test_vi_comment(self):
        self._test_encoding(u"""
日本語 = 42
puts(日本語)

# vim: fileencoding=Shift-JIS
""", """42
""")

    def test_magic_comment1(self):
        self._test_encoding(u"""# -*- coding: Shift-JIS -*-

日本語 = 42
puts(日本語)
""", """42
""")

    def test_magic_comment2(self):
        self._test_encoding(u"""#!/usr/bin/yog
# -*- coding: Shift-JIS -*-

日本語 = 42
puts(日本語)
""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
