# -*- coding: utf-8 -*-

from shutil import rmtree
from testcase import TestCase, find_so

class TestO(TestCase):

    disabled = not find_so("o")
    options = []

    def get_db_path(self):
        return "TestO.db"

    def test_gc0(self):
        self._test("import o", options=["--gc-stress"])

    def clean_db(self, path):
        rmtree(path, ignore_errors=True)

    def test_search0(self):
        path = self.get_db_path()
        self.clean_db(path)
        self._test("""
import o
enable_gc_stress()
db = o.oDB.new()
o.oDB_init(db)
try
  o.oDB_create(db, \"%(path)s\")
  o.oDB_open_to_write(db, \"%(path)s\")
  try
    o.oDB_put(db, \"foobarbazquuxhogepiyofuga\")
  finally
    o.oDB_close(db)
  end
  o.oDB_open_to_read(db, \"%(path)s\")
  try
    ptr = Pointer.new()
    o.oDB_search(db, \"uu\", ptr)
    try
      hits = o.oHits.new(ptr)
      print(hits.num)
    finally
      libc = load_lib(\"/lib/tls/i686/cmov/libc.so.6\")
      free = libc.load_func(\"free\", [\'pointer])
      free(ptr)
    end
  finally
    o.oDB_close(db)
  end
finally
  o.oDB_fini(db)
end
""" % locals(), "1")

    def test_search10(self):
        path = self.get_db_path()
        self.clean_db(path)
        self._test("""
import o
enable_gc_stress()
db = o.oDB.new()
o.oDB_init(db)
try
  o.oDB_create(db, \"%(path)s\")
  o.oDB_open_to_write(db, \"%(path)s\")
  try
    o.oDB_put(db, \"foobarbazquuxhogepiyofuga\")
  finally
    o.oDB_close(db)
  end
  o.oDB_open_to_read(db, \"%(path)s\")
  try
    ptr = Pointer.new()
    o.oDB_search(db, \"uu\", ptr)
    try
      hits = o.oHits.new(ptr)
      print(hits.doc_id[0])
    finally
      libc = load_lib(\"/lib/tls/i686/cmov/libc.so.6\")
      free = libc.load_func(\"free\", [\'pointer])
      free(ptr)
    end
  finally
    o.oDB_close(db)
  end
finally
  o.oDB_fini(db)
end
""" % locals(), "0")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
