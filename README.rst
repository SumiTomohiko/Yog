
Yog
***

Yog is a dynamic programming language designed to have Python's usefully and
look like Ruby.

Features
========

* Syntax like Python and Ruby
* Class based object oriented
* Garbage collection
* under MIT license

Requirements
============

* FreeBSD 9.1/amd64

Previous versions supported Ubuntu Linux and OpenBSD, but this version does not.
Future version will support again.

Documentation
=============

Online documents are at
http://neko-daisuki.ddo.jp/~SumiTomohiko/yog/doc/0.26.0/index.html.

Offline documents are available in the ``doc/html`` directory.

Download
========

.tar.xz are available at `author's repository <http://neko-daisuki.ddo.jp/~SumiTomohiko/repos/index.html>`_.

Build Instructions
==================

::

  $ ./configure && make

Issue Tracking System
=====================

You can see the issues at http://neko-daisuki.ddo.jp/~SumiTomohiko/yog/issues/index.html.
Original data are in the ``issues`` dicretory of the source tree.

Plan
====

version 1.0.0
-------------

* GPL free
  * `waf <http://code.google.com/p/waf/>`_'ize to remove autotools
  * Change the compiler to `llvm clang <http://clang.llvm.org/>`_
  * Change GMP to a non-copyleft library.
* Supports Linux and OpenBSD.

Author
======

The author of Yog is
`SumiTomohiko <http://neko-daisuki.ddo.jp/~SumiTomohiko/index.html>`_.

.. vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
