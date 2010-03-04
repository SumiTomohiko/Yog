#!/bin/sh

aclocal
autoconf
automake --add-missing
autoheader

# vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
