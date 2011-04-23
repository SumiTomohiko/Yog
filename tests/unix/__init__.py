
import os
import pytest
from testcase import TestCase

@pytest.mark.skipif("os.name != \"posix\"")
class TestUnix(TestCase):
    pass

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
