# -*- coding: utf-8 -*-

from tests.encoding import TestEncoding

class TestEucJp(TestEncoding):

    encoding = "EUC-JP"

    def test_vi_comment(self):
        self._test_encoding(u"""
日本語 = 42
puts 日本語

# vim: fileencoding=EUC-JP
""", """42
""")

    def test_magic_comment1(self):
        self._test_encoding(u"""# -*- coding: EUC-JP -*-

日本語 = 42
puts 日本語
""", """42
""")

    def test_magic_comment2(self):
        self._test_encoding(u"""#!/usr/bin/yog
# -*- coding: EUC-JP -*-

日本語 = 42
puts 日本語
""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
