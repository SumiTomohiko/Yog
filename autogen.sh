#!/bin/sh

aclocal
autoheader
autoconf
automake --add-missing
(cd tools/swig && ./autogen.sh)

# vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
