
import os
from testcase import TestCase

class TestUnix(TestCase):

    disabled = os.name != "posix"

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
