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
If a statement is incomplete, the interpreter prompts continuation lines with secondary prompt ``...``::

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

You can make scripts directly executable by putting the following line at the head of the file,::

  #!/usr/local/bin/yog

and add execute mode to the file by ``chmod`` command.::

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
So the folowing two examples are valid::

  # vim: fileencoding=EUC-JP
  # -*- coding: EUC-JP -*-

*******************************
An Informal Introduction to Yog
*******************************

Using Yog as a Calculator
=========================

Numbers
-------

Strings
-------

Arrays
------

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

::
  vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
