#!/bin/sh

aclocal
autoheader
autoconf
automake --add-missing

# vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
