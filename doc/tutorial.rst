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

Usually Yog is installed as ``/usr/local/bin/yog``. If ``/usr/local/bin`` is in your ``PATH`` environment variable, you can invoke Yog interpreter for::

  $ yog

This command invoke Yog in interactive mode (see below). To exit interactive mode, type end of file ``Ctrl+D``.

Argument Passing
================

Interactive Mode
================

***********************************
The Interpreter and Its Environment
***********************************

Executable Yog Scripts
======================

Source Code Encoding
====================

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
