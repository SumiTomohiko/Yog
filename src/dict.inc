"\n"
"def to_s()\n"
"  def f(obj)\n"
"    return \"{...}\"\n"
"  end\n"
"\n"
"  def g(obj)\n"
"    a = []\n"
"    obj.each() do [key, value]\n"
"      a.push(key.inspect() + \" => \" + value.inspect())\n"
"    end\n"
"    return \"{ \" + a.join(\", \") + \" }\"\n"
"  end\n"
"\n"
"  return __recurse__(self, f, g)\n"
"end\n"
"\n"