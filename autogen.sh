#!/bin/sh

run()
{
  "$@"
  if [ $? != 0 ]; then
    echo "$@ failed"
    exit 1
  fi
}

run aclocal
run autoheader
run autoconf
run automake --add-missing
(cd tools/swig && ./autogen.sh)

# vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
