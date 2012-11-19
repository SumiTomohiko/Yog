# -*- coding: utf-8 -*-

from testcase import TestCase, enumerate_tuples

class TestBinary(TestCase):

    def test_inspect0(self):
        src = "print(\"foo\".to_bin(ENCODINGS[\"utf-8\"]).inspect())"
        self._test(src, "b\"\\x66\\x6f\\x6f\\x00\"")

    def test_to_s0(self):
        src = "print(\"foo\".to_bin(ENCODINGS[\"utf-8\"]).inspect())"
        self._test(src, "b\"\\x66\\x6f\\x6f\\x00\"")
        self._test("""\
enc = ENCODINGS[\"utf-8\"]
print(\"foo\".to_bin(enc).to_s(enc))""", "foo")

    for i, data in enumerate(([], [42], [42, 26])):
        exec("""def test_lshift_number{i}(self):
    self._test(\"\"\"bin = Binary.new()
{data}.each() do |elem|
  bin << elem
end
print(bin.to_a())\"\"\", {expected})
""".format(i=10 * i, data=data, expected=repr(data)))

    def test_lshift_binary0(self):
        self._test("""bin1 = Binary.new()
bin2 = Binary.new()
bin1 << bin2
print(bin1.to_a())""", "[]")

    def test_lshift_binary10(self):
        self._test("""bin1 = Binary.new()
bin2 = Binary.new()
bin2 << 42
bin1 << bin2
print(bin1.to_a())""", "[42]")

    def test_lshift_binary20(self):
        self._test("""bin1 = Binary.new()
bin2 = Binary.new()
bin2 << 42
bin2 << 26
bin1 << bin2
print(bin1.to_a())""", "[42, 26]")

    for i, data in enumerate(([], [42], [42, 26])):
        exec("""def test_size{i}(self):
    self._test(\"\"\"bin = Binary.new()
{data}.each() do |elem|
  bin << elem
end
print(bin.size)\"\"\", \"{expected}\")
""".format(i=10 * i, data=data, expected=len(data)))

    for i, data, index in enumerate_tuples((
            ([42], 0),
            ([42], -1),
            ([42, 26], 0),
            ([42, 26], 1),
            ([42, 26], -1))):
        exec("""def test_subscript{i}(self):
    self._test(\"\"\"bin = Binary.new()
{data}.each() do |elem|
  bin << elem
end
print(bin[{index}])\"\"\", \"{expected}\")
""".format(i=10 * i, data=data, index=index, expected=data[index]))

    for i, data, index in enumerate_tuples((
            ([], 0),
            ([], -1),
            ([42], 1),
            ([42], -2),
            ([42, 26], 2),
            ([42, 26], -3))):
        exec("""def test_subscript_error{i}(self):
    index = {index}
    size = len({data})
    fmt = \"Binary index out of range: size is {{size}}, but {{index}} given\"
    self._test_exception(\"\"\"bin = Binary.new()
{data}.each() do |elem|
  bin << elem
end
bin[{index}]\"\"\", \"IndexError\", fmt.format(**locals()))
""".format(i=10 * i, index=index, data=data))

    for i, data in enumerate(([], [42], [42, 26])):
        exec("""def test_each{i}(self):
    self._test(\"\"\"bin = Binary.new()
{data}.each() do |elem|
  bin << elem
end
bin.each() do |elem|
  print(elem)
end\"\"\", {expected})
""".format(i=10 * i, data=data, expected=repr("".join([str(n) for n in data]))))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=python
