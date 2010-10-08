#!/bin/sh

run()
{
  "$@"
  if [ $? != 0 ]; then
    echo "$@ failed"
    exit 1
  fi
}

run mkdir -p m4
run autoreconf --install
run automake --add-missing

# vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
