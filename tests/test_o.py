# -*- coding: utf-8 -*-

from shutil import rmtree
import pytest
from testcase import TestCase, find_so

@pytest.mark.skipif("not find_so(\"o\")")
class TestO(TestCase):

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
        self._test("""from libc.memory import free
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
      hits = o.oHits.new(ptr.value)
      print(hits.num)
    finally
      free(ptr.value)
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
        self._test("""from libc.memory import free
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
      hits = o.oHits.new(ptr.value)
      print(hits.doc_id[0])
    finally
      free(ptr.value)
    end
  finally
    o.oDB_close(db)
  end
finally
  o.oDB_fini(db)
end
""" % locals(), "0")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
