# -*- coding: utf-8 -*-

from testcase import TestCase
from utils import is_32bit

class TestArch32(TestCase):

    disabled = is_32bit() is not True

    def test_string_multiply0(self):
        pass

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
