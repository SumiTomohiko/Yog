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

    def do_lalr1gram(self, gram):
        path = self.write_content_to_tmpfile(gram, ".yogram")
        try:
            script = join(environ["BIN_DIR"], "lalr1gram.yog")
            cmd = [get_command(), script, "gram.yg", path]
            proc = Popen(cmd, stdout=PIPE, stderr=PIPE)
            proc.wait()
        finally:
            unlink(path)
        return proc

    def do_generator_test(self, src, test_func):
        proc = self.do_lalr1gram(src)
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
foo -> (: empty :) {
}
;
""", self.check_normal_lalr1gram_result)

    def test_comment2(self):
        self.do_generator_test("""
foo -> <bar> {
}
;
(:
 : special comments
 :)
""", self.check_normal_lalr1gram_result)

    def write_content_to_tmpfile(self, content, suffix):
        path = self.make_temp_file(suffix=suffix)
        try:
            with open(path, "w") as fp:
                fp.write(content)
        except:
            unlink(path)
        return path

    def do_parser_test(self, gram, src, expected):
        self.do_lalr1gram(gram)
        path = self.write_content_to_tmpfile(src, ".yg")
        try:
            proc = Popen([get_command(), path], stdout=PIPE, stderr=PIPE)
            proc.wait()
        finally:
            unlink(path)
        assert expected == proc.stdout.read()

    def test_parser0(self):
        self.do_parser_test("""
foo -> <bar> {
  return 42
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'bar)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "42")

    def test_parser10(self):
        self.do_parser_test("""
foo -> <bar> {
  return bar.value
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'bar, 42)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "42")

    def test_parser20(self):
        self.do_parser_test("""
foo -> <bar> <baz> {
  return bar.value
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'bar, 42)
    end
    if n == 2
      return Token.new(\'baz, 26)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "42")

    def test_parser30(self):
        self.do_parser_test("""
foo -> <bar> <baz> {
  return baz.value
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'bar, 42)
    end
    if n == 2
      return Token.new(\'baz, 26)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "26")

    def test_parser40(self):
        self.do_parser_test("""
foo -> bar {
  return bar
}
;
bar -> <baz> {
  return 42
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'baz, 42)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "42")

    def test_parser50(self):
        self.do_parser_test("""
foo -> bar {
  return bar
}
;
bar -> <baz> {
  return baz.value
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'baz, 42)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "42")

    def test_parser60(self):
        self.do_parser_test("""
foo -> bar@<baz> quux@<baz> {
  return bar.value
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'baz, 42)
    end
    if n == 2
      return Token.new(\'baz, 26)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "42")

    def test_parser70(self):
        self.do_parser_test("""
foo -> bar@<baz> quux@<baz> {
  return quux.value
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'baz, 42)
    end
    if n == 2
      return Token.new(\'baz, 26)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "26")

    def test_parser80(self):
        self.do_parser_test("""
foo -> <bar> {
  return bar.value
}
    | <baz> {
  return baz.value
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'bar, 42)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "42")

    def test_parser90(self):
        self.do_parser_test("""
foo -> <bar> {
  return bar.value
}
    | <baz> {
  return baz.value
}
;
""", """
from lalr1 import Token, parse
import gram

def get_get_token()
  n = 0
  def get_token()
    nonlocal n
    n += 1
    if n == 1
      return Token.new(\'bar, 42)
    end
    return nil
  end
  return get_token
end

print(parse(gram, get_get_token()))
""", "42")

    def test_include0(self):
        self.do_parser_test("""
%{
def foo()
  return 42
end
%}
bar -> {
}
;
""", """
from gram import foo
print(foo())
""", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
