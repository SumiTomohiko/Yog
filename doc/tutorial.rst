############
Yog Tutorial
############

Yog is a dynamic programming language designed as intermediate of Python and Ruby.

This document introduces basic features of Yog.

***********
Requirement
***********

Yog needs Linux on Intel i386 architecture.

**************
Installing Yog
**************

You can download Yog's source code from http://github.com/SumiTomohiko/Yog/downloads.

The following commands will extract source code, build and install Yog::

  $ tar xf Yog-0.0.1.tar.gz
  $ cd Yog-0.0.1
  $ ./configure
  $ make
  $ sudo make install

*********************
Using Yog Interpreter
*********************

Invoke Interpreter
==================

Usually Yog is installed as ``/usr/local/bin/yog``.
If ``/usr/local/bin`` is in your ``PATH`` environment variable, you can invoke Yog interpreter for::

  $ yog

This command invokes Yog in interactive mode (see below).
To exit interactive mode, type end of file ``Ctrl+D``.

If you want to execute a script, provide script's file name to ``yog`` as an argument::

  $ yog filename

Interactive Mode
================

When you invoke the interpreter without script file name, the interpreter works in *interactive mode*.
In this mode, the interpreter waits a statement with primary prompt ``>>>``.
You can give a statement to see the result immediately.
If a statement is incomplete, the interpreter prompts continuation lines with secondary prompt ``...`::

  $ yog
  >>> if true
  ...   puts(42)
  ... end
  42

When the statement is an expression, the interpreter displays the return value of that expression with leading ``=>``::

  $ yog
  >>> puts(42)
  42
  => nil

***********************************
The Interpreter and Its Environment
***********************************

Executable Yog Scripts
======================

You can make scripts directly executable by putting the following line at the head of the file::

  #!/usr/local/bin/yog

and add execute mode to the file by ``chmod`` command::

  $ chmod +x filename

Source Code Encoding
====================

By default, the Yog interpreter assumes that source code is written in UTF-8.
When you use any other encodings, you need to tell that code to the interpreter.
This is done by putting the special comment like::

  # coding: EUC-JP

The interpreter accepts the encoding of ``EUC-JP``, ``Shift-JIS`` and ``UTF-8``.
Leading charactors and trailing charactors are allowed in this comment.
And you can use "``=``" in place of "``:``".
So the folowing two examples are llegal::

  # vim: fileencoding=EUC-JP
  # -*- coding: EUC-JP -*-

*******************************
An Informal Introduction to Yog
*******************************

Using Yog as a Calculator
=========================

Let's try simple script.
This section show you some examples in interactive mode.

Numbers
-------

Numbers like ``42`` are evaluated as be seen::

  >>> 42
  => 42

Numbers starts with ``0b`` are evaluated as binary::

  >>> 0x101010
  => 42

Samely, numbers which starts with ``0o`` are octal, and ``0x`` are hex::

  >>> 0o52
  => 42
  >>> 0x2a
  => 42

You can split a number by ``_``::

  >>> 0x10_1010
  => 42

The interpreter accepts floating point number::

  >>> 3.1415926535
  => 3.14159

The operators for numbers of ``+``, ``-`` and ``*`` are normally defined::

  >>> 42 + 26
  => 68
  >>> 42 - 26
  => 16
  >>> 42 * 26
  => 1092

Division
~~~~~~~~

Yog has two division operators, one is ``/``, the other is ``//``.
In almost programming language like C, ``3 / 2`` gives 1 (not 1.5).
In Yog, the operator to work this way is ``//``.
The normal ``/`` operator gives always floating point number, so ``3 / 2`` gives 1.5::

  >>> 3 // 2
  => 1
  >>> 3 / 2
  => 1.5

Assignment
~~~~~~~~~~

The symbol ``=`` is used to assign a value to a variable.
You can use assigned variables in continuation script::

  >>> foo = 42
  => 42
  >>> bar = 26
  => 26
  >>> foo * bar
  => 1092

When unassigned variable is used, an error will occur::

  >>> baz
  Traceback (most recent call last):
    File "__main__", line 1, in <package>
  NameError: name 'baz' is not defined

Strings
-------

String can be enclosed in double quotes::

  >>> "Cthulhu ftgn! ftgn!"
  => Cthulhu ftgn! ftgn!
  >>> "\"You, fool, Warren is DEAD!\""
  => "You, fool, Warren is DEAD!"

The operator ``+`` concatenates two strings::

  >>> baz = "foo" + "bar"
  => foobar

Strings can be indexed.
Indexing strings accesses each charactors, not bytes::

  >>> baz[0]
  => f
  >>> quux = "九頭龍"
  => 九頭龍
  >>> quux[0]
  => 九

Unlike strings in some script languages, Yog's strings are mutable.
You can overwrite a charactor by assigning to a indexed string::

  >>> hoge = "piyo"
  => piyo
  >>> hoge[0] = "P"
  => P
  >>> hoge
  => Piyo

Arrays
------

Yog supports most basic data scructure, arrays.
Enclosing comma separated elements by brackets makes an array::

  >>> a = [42, "foo", 3.1415926545]
  => [42, "foo", 3.14159]

Indexing an array reads/writes each elements::

  >>> a[0]
  => [42]
  >>> a[1] = "bar"
  => [42, "bar", 3.14159]

Arrays have ``size`` property, it is number of elements in the array::

  >>> a.size
  => 3

Like strings, arrays can be concatenated by the operator ``+``::

  >>> b = [26, "baz"]
  => [26, "baz"]
  >>> c = a + b
  => [42, "bar", 3.14159, 26, "baz"]

You can remove/append the last element of arrays by ``pop``/``push`` method::

  >>> c.pop()
  => baz
  >>> c.push("quux")
  => [42, "bar", 3.14159, 26, "quux"]

First Steps Towrards Programming
================================

***********************
More Control Flow Tools
***********************

``if`` Statements
=================

no ``for`` Statements
=====================

``break`` and ``next`` Statements
=================================

Defining Functions
==================

More on Defining Functions
==========================

Keyword Arguments
=================

Arbitrary Argument Arrays
=========================

Unpacking Argument Arrays
=========================

***************
Data Structures
***************

More on Arrays
==============

Using Arrays as Stacks
======================

Using Arrays as Queues
======================

Sets
====

Dictionaries
============

********
Packages
********

More on Packages
================

Executing packages as scripts
-----------------------------

The Package Search Path
-----------------------

Package Hierarchy
=================

****************
Input and Output
****************

Reading and Writing Files
=========================

Methods of File Objects
=======================

*********************
Errors and Exceptions
*********************

Syntax Errors
=============

Exceptions
==========

Handling Exceptions
===================

Raising Exceptions
==================

User-defined Exceptions
=======================

Defining Clean-up Actions
=========================

*******
Classes
*******

A First Look at Classes
=======================

Class Definition Syntax
-----------------------

Class Objects
-------------

Instance Objects
----------------

Method Objects
--------------

Inheritance
===========

Mix-in
======

.. vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
