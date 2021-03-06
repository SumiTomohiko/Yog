
corgi 1.0.0dev1
===============

corgi is a regular expression library for C.

Features
--------

* corgi accepts UTF-32 strings.
* Based on Python's regular expression's engine (it seems fast).
* MIT License

Requirements
------------

* FreeBSD 9.0/amd64
* FreeBSD 9.0/i386

Building and Installing
-----------------------

Prerequirements
~~~~~~~~~~~~~~~

* `Python 3 <http://www.python.org/>`_

Building Instruction
~~~~~~~~~~~~~~~~~~~~

::

  $ ./configure && make

You will get ``libcorgi.so``, ``libcorgi.a`` and ``corgi`` in ``build/src``
directory.

Installing Instruction
~~~~~~~~~~~~~~~~~~~~~~

::

  $ make install

``corgi`` Utility
-----------------

``corgi`` in ``src`` directory is a utility to try corgi. It can match a string
with a regular expression to show a matched substring and disassemble VM codes
of a regular expression.

``corgi``'s usage is::

  corgi [OPTIONS]... SUBCOMMAND [PARAMETERS]...

``SUBCOMMAND`` is one of the followings.

* ``match``
* ``search``
* ``disassemble``

``match`` Subcommand
~~~~~~~~~~~~~~~~~~~~

``match`` subcommand trys to match a string with a regular expression. When a
string is matched with a regular expression, ``corgi`` shows a matched part of
the string. ``match`` subcommand's usage is::

  corgi [OPTIONS]... match <string> <regexp>

``OPTIONS`` are

* ``--group-id``: group number to show
* ``--ignore-case``: ignore case

For example::

  $ src/corgi --group-id 1 '<title>(.*)</title>' '<title>Hello, corgi</title>'

will show you::

  Hello, corgi

``search`` Subcommand
~~~~~~~~~~~~~~~~~~~~~

``search`` subcommand searches a substring of a string which matches with a
regular expression. ``search`` subcommand differs from ``match`` subcommand. In
``match`` subcommand, a string must match with a regular expression at the
beginning of the string. But in ``search`` subcommand, a string can match at
anywhere.

``search`` subcommand's usage is the same as that of ``match`` subcommand.

``disassemble`` Subcommand
~~~~~~~~~~~~~~~~~~~~~~~~~~

``disassemble`` subcommand shows you VM codes of a given regular expression. For
example,::

  $ src/corgi disassemble "foo"
  0000 LITERAL      102 (f)
  0002 LITERAL      111 (o)
  0004 LITERAL      111 (o)
  0006 SUCCESS

``disassemble`` subcommand's usage is::

  corgi diassemble <regexp>

Syntax
------

Syntax Elements
~~~~~~~~~~~~~~~

================ ===============
``|``            alternation
``(...)``        group
``(?<name>...)`` named group
``[...]``        character class
================ ===============

Character Types
~~~~~~~~~~~~~~~

====== ==================================================================
``\w`` word character (Letter \| Mark \| Number \| Connector-Punctuation)
``\W`` non word character
``\s`` white space character (``0x09``, ``0x0a``, ``0x0b``, ``0x0c``, ``0x0d``, ``0x85``)
``\S`` no white space character
``\d`` decimal number (GeneralCategory - DecimalNumber)
``\D`` non decimal digit character
====== ==================================================================

Quantifier
~~~~~~~~~~

greedy
^^^^^^

========= ================================================
``?``     1 or 0 times
``*``     0 or more times
``+``     1 or more times
``{n,m}`` at least n but not more than m times
``{n,}``  at least n times
``{,n}``  at least 0 but not more than n times (``{0,n}``)
``{n}``   n times
========= ================================================

reluctant
^^^^^^^^^

========== ====================================================
``??``     1 or 0 times
``*?``     0 or more times
``+?``     1 or more times
``{n,m}?`` at least n but not more than m times
``{n,}?``  at least n times
``{,n}?``  at least 0 but not more than n times (== ``{0,n}?``)
========== ====================================================

Anchors
~~~~~~~

====== ===========================================
``^``  beginning of the line
``$``  end of the line
``\b`` word boundary
``\B`` not word boundary
``\A`` beginning of string
``\Z`` end of string, or before newline at the end
====== ===========================================

Character class
~~~~~~~~~~~~~~~

========= ===========================================
``^...``  negative class (lowest precedence operator)
``x-y``   range from x to y
``[...]`` set (character class in character class)
========= ===========================================

API
---

Header File
~~~~~~~~~~~

You must include ``corgi.h`` in ``include`` directory::

  #include <corgi.h>

Data Types and Structures
~~~~~~~~~~~~~~~~~~~~~~~~~

.. c:type:: CorgiChar

:c:type:`CorgiChar` represents one character of UTF-32.

.. c:type:: CorgiStatus

Type of corgi API's return values is :c:type:`CorgiStatus`.  When they work
successfully, they return :c:data:`CORGI_OK`. You can convert
:c:type:`CorgiStatus` values to its string representation by
:c:func:`corgi_strerror`.

.. c:type:: CorgiUInt

This is an unsigned integer whose size is same as pointers.

.. c:type:: CorgiRegexp

:c:type:`CorgiRegexp` represents a regular expression. This must be initialized
by :c:func:`corgi_init_regexp`, and must be cleaned up by
:c:func:`corgi_fini_regexp`.

.. c:type:: CorgiMatch

:c:type:`CorgiMatch` is matching information. You must initialize this with
:c:func:`corgi_init_match`, and clean up with
:c:func:`corgi_fini_match`.

.. c:member:: CorgiUInt CorgiMatch::begin

Starting position of a matched part in the string.

.. c:member:: CorgiUInt CorgiMatch::end

Ending position of a matched part in the string.

.. c:type:: CorgiOptions

Variables of this data type are to contain flags. The followings flags are
allowed.

=============================== ===========
:c:data:`CORGI_OPT_IGNORE_CASE` Ignore case
=============================== ===========

Functions
~~~~~~~~~

.. c:function:: CorgiStatus corgi_compile(CorgiRegexp* regexp, CorgiChar* begin, CorgiChar* end, CorgiOptions opts)

Compiles a regular expression and contains results to *regexp*. *begin* is a
pointer to beginning of the regular expression, and *end* is a pointer to end.

.. c:function:: CorgiStatus corgi_disassemble(CorgiRegexp* regexp)

Prints VM codes of a regular expression to standard output.

.. c:function:: CorgiStatus corgi_fini_match(CorgiMatch* match)

Cleans up data in *match*.

.. c:function:: CorgiStatus corgi_fini_regexp(CorgiRegexp* regexp)

Cleans up data in *regexp*.

.. c:function:: CorgiStatus corgi_get_group_range(CorgiMatch* match, CorgiUInt group_id, CorgiUInt* begin, CorgiUInt* end)

Sets range of a group of *group_id* to *begin* and *end*.

.. c:function:: CorgiStatus corgi_group_name2id(CorgiRegexp* regexp, CorgiChar* begin, CorgiChar* end, CorgiUInt* group_id)

Converts a group name starting from *begin* to an index.

.. c:function:: CorgiStatus corgi_init_match(CorgiMatch* match)

Sets up *match*.

.. c:function:: CorgiStatus corgi_init_regexp(CorgiRegexp* regexp)

Sets up *regexp*.

.. c:function:: CorgiStatus corgi_match(CorgiMatch* match, CorgiRegexp* regexp, CorgiChar* begin, CorgiChar* end, CorgiChar* at, CorgiOptions opts)

Trys to match *regexp* with a string which starts from *begin* and ends at
*end*. Matching is started at *at*. When the string matches with *regexp*,
:c:func:`corgi_match` returns :c:data:`CORGI_OK`. If the string doesn't match
with *regexp*, :c:func:`corgi_match` returns :c:data:`CORGI_MISMATCH`.

.. c:function:: CorgiStatus corgi_search(CorgiMatch* match, CorgiRegexp* regexp, CorgiChar* begin, CorgiChar* end, CorgiChar* at, CorgiOptions opts)

Searches *regexp* in a string which starts from *begin* and ends at *end*.
Searching is started from *at*.

.. c:function:: const char* corgi_strerror(CorgiStatus status)

Converts a :c:type:`CorgiStatus` value to a string.

Author
------

- `Tomohiko Sumi <http://neko-daisuki.ddo.jp/~SumiTomohiko/index.html>`_

.. vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2 filetype=rst
