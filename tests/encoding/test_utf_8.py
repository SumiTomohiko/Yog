# -*- coding: utf-8 -*-

from encoding import TestEncoding

class TestUtf8(TestEncoding):

    encoding = "UTF-8"

    def test_vi_comment(self):
        self._test_encoding(u"""
日本語 = 42
puts 日本語

# vim: fileencoding=utf-8
""", """42
""")

    def test_magic_comment1(self):
        self._test_encoding(u"""# -*- coding: utf-8 -*-

日本語 = 42
puts 日本語
""", """42
""")

    def test_magic_comment2(self):
        self._test_encoding(u"""#!/usr/bin/yog
# -*- coding: utf-8 -*-

日本語 = 42
puts 日本語
""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
