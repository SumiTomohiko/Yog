# -*- coding: utf-8 -*-

from os import environ, unlink
from os.path import join
from subprocess import PIPE, Popen
from testcase import TestCase, get_command

class TestLalr1gram(TestCase):

    def check_normal_lalr1gram_result(self, status, stdout, stderr):
        assert status == 0
        assert stdout == ""
        assert stderr == ""

    def do_generator_test(self, src, test_func):
        path = self.make_temp_file(suffix=".yogg")
        try:
            with open(path, "w") as fp:
                fp.write(src)
            script = join(environ["BIN_DIR"], "lalr1gram.yog")
            cmd = [get_command(), script, "gram.yog", path]
            proc = Popen(cmd, stdout=PIPE, stderr=PIPE)
            proc.wait()
        finally:
            unlink(path)
        test_func(proc.returncode, proc.stdout.read(), proc.stderr.read())

    def test_basic_tokens1(self):
        self.do_generator_test("""
foo -> <bar> {
}
;
""", self.check_normal_lalr1gram_result)

    def test_basic_tokens2(self):
        self.do_generator_test("""
foo -> bar {
}
;
bar -> <baz> {
}
;
""", self.check_normal_lalr1gram_result)

    def test_basic_tokens3(self):
        self.do_generator_test("""
foo -> baz@<bar> {
}
;
""", self.check_normal_lalr1gram_result)

    def test_basic_tokens4(self):
        self.do_generator_test("""
foo -> baz@bar {
}
;
bar -> <quux> {
}
;
""", self.check_normal_lalr1gram_result)

    def test_basic_tokens5(self):
        self.do_generator_test("""
foo -> <bar> {
}
    | <baz> {
}
;
""", self.check_normal_lalr1gram_result)

    def test_comment1(self):
        self.do_generator_test("""
foo -> /* empty */ {
}
;
""", self.check_normal_lalr1gram_result)

    def test_comment2(self):
        self.do_generator_test("""
foo -> <bar> {
}
;
/**
 * special comments
 */
""", self.check_normal_lalr1gram_result)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
