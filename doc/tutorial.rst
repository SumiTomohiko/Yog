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
Leading characters and trailing characters are allowed in this comment.
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

  >>> foo = "Yog" + "Sothoth"
  => YogSothoth

Strings can be indexed.
Indexing a string reads each characters, not bytes::

  >>> foo[0]
  => Y
  >>> bar = "九頭龍"
  => 九頭龍
  >>> bar[0]
  => 九

Unlike strings in some script languages, Yog's strings are mutable.
You can overwrite a character by assigning to a indexed string::

  >>> baz = "dagon"
  => dagon
  >>> baz[0] = "D"
  => D
  >>> baz
  => Dagon

Yog can append a string with the ``<<`` operator::

  >>> s = "Creep!"
  => Creep!
  >>> s << " Nyalathotep Girl"
  => Creep! Nyalathotep Girl
  >>> s
  => Creep! Nyalathotep Girl

Regular Expressions
-------------------

As same as Perl and Ruby, Yog supports regular expressions' literal.
Regular expressions are enclosed in ``/``.
For example, ``/Innsmouth/`` is a regular expression.
In regular expressions, backslash ``\`` doen't escape characters excepting ``\n`` (it is a newline) and ``\\`` (it is a backslash itself).
For instance, ``/H\.P\.Lovecraft/`` matches ``"H.P.Lovecraft"``.

To test if a string matches a regular expression, use the ``=~`` operator.
This operator returns a ``Match`` object when the string matches, or returns ``nil`` when doesn't match.

``Match`` objects have ``group`` method.
When ``group`` method is called without arguments, it returns a matched part of the string::

  >>> m = ("carter@example.com" =~ /([a-z]+)@([a-z.]+)/)
  => <Match 0000000000000289>
  >>> m.group()
  => carter@example.com

When ``group`` method is called with an integer argument, it returns a part of the string corresponding to the group in the regular expression. The first group's index is one::

  >>> m.group(1)
  => carter
  >>> m.group(2)
  => example.com

``Match`` objects have ``start`` method and ``end`` method. ``start`` method returns a start position of a matched part in the string, ``end`` method returns a end position.
Calling ``start`` method and ``end`` method with argument ``0`` is same as calling without arguments::

  >>> m.start()
  => 0
  >>> m.start(0)
  => 0
  >>> m.start(1)
  => 0
  >>> m.start(2)
  => 7
  >>> m.end()
  => 18
  >>> m.end(0)
  => 18
  >>> m.end(1)
  => 6
  >>> m.end(2)
  => 18

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

You can also append an element with the ``<<`` operator::

  >>> c << "hoge"
  => [42, "bar", 3.14159, 26, "quux", "hoge"]
  >>> c
  => [42, "bar", 3.14159, 26, "quux", "hoge"]

******************
Control Flow Tools
******************

Yog has set of control structure.
``if`` is for conditional, ``while`` is for iteration.

``if`` Statements
=================

One example of ``if`` statement is::

  >>> n = 42
  => 42
  >>> if 0 < n
  ...   puts("positive")
  ... elif n == 0
  ...   puts("zero")
  ... else
  ...   puts("negative")
  ... end
  positive

Yog uses the ``elif`` keyword to place the condition sequence.

There can be zero or more ``elif`` parts.
The ``else`` part is optional.

``while`` Statements
====================

One example of ``while`` statement is::

  >>> n = 0
  => 0
  >>> while n < 42
  ...   print("Yeh")
  ...   n += 1
  ... end
  YehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYeh

``break`` and ``next`` Statements
=================================

The ``break`` statement ends most internal ``while`` loop.

The ``next`` statement starts next iteration of the loop.

no ``for`` Statements
=====================

Many languages have the ``for`` statement to iterate,  but Yog DOESN'T HAVE THE ``for`` STATEMENT.
Yog's objects have methods to iterate, so Yog doesn't need the ``for`` statement.

To iterate for some times, you can use ``times`` method of integer::

  >>> 42.times() do
  ...   print("Yeh")
  ... end
  YehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYehYeh=> nil

The sequence of statements enclosed between ``do`` and ``end`` is one object for ``times`` method, so called *block*.
``times`` method calls this block for each times in the iteration.

To do something for all elements in an array, you can use ``each`` method::

  >>> [42, 26].each() do [n]
  ...   puts(n)
  ... end
  42
  26
  => [42, 26]

Blocks can have formal parameters.
When declaring formal parameters, enclose variables by brackets ``[`` and ``]``.
In this case, ``n`` is the formal parameter.
The ``each`` method sets the each array's element to this parameter when calling block.

Defining Functions
==================

Defining functions in Yog are like this::

  >>> def fib(n)
  ...   if (n == 0) || (n == 1)
  ...     return 1
  ...   else
  ...     return fib(n - 1) + fib(n - 2)
  ...   end
  ... end
  >>> fib(16)
  => 1597

Defining a function starts from the ``def`` keyword.
The function name and the parenthesized formal parameters follows the ``def`` keyword.
The function body starts at the next line, and the function must end with keyword ``end``.

More on Defining Functions
==========================

Keyword Arguments
=================

You can pass arguments to functions in the form of ``name: value``.
For instance, assume this function::

  def markup(text, bold, italic)
    s = text
    if bold
      s = "<b>" + s + "</b>"
    end
    if italic
      s = "<em>" + s + "</em>"
    end
    return s
  end

You can pass ``"Randolph Cartar`` for the argument ``text``, ``false`` for ``bold`` and ``true`` for ``italic`` to this function in the following ways::

  markup("Randolph Carter", false, true)
  markup("Randolph Carter", false, italic: true)
  markup("Randolph Carter", italic: true, bold: false)
  markup(italic: true, text: "Randolph Carter", bold: false)

Keyword arguments must be placed after the positional arguments.

Arbitrary Argument Arrays
=========================

One of the useful functions is an arbitrary number of arguments.
If you put the argument ``*args`` (``args`` can be the name which you like), this becomes the variable argument.
``args`` is an array.
When you define the following function::

  >>> def foo(*args)
  ...   args.each() do [n]
  ...     puts(n)
  ...   end
  ... end

You can call this function with an arbitrary number of arguments::

  >>> foo(42, 26)
  42
  26
  => nil

If actual parameters are specified by name, these are stored to the argument ``**kw`` (``kw`` can be the name you like).
``kw`` is a dictionary.
When you define the following function::

  >>> def foo(*kw)
  ...   kw.each() do [key, value]
  ...     print(key)
  ...     print(":")
  ...     print(value)
  ...     print("\n")
  ...   end
  ... end

You can call this function with an arbitrary number of keyword arguments::

  >>> foo(bar: 42, baz: 26)
  bar:42
  baz:26

Unpacking Argument Arrays
=========================

If you have an array ``args`` in which arguments are stored, you can use ``*args`` to unpack this array::

  >>> def add(n, m)
  ...   return n + m
  ... end
  >>> a = [42, 26]
  => [42, 26]
  >>> add(*a)
  => 68

If you have a dictionary ``kw`` in which arguments are stored, you can use ``**kw`` to unpack this dictionary to keyword arguments::

  >>> def add(n, m)
  ...   return n + m
  ... end
  >>> d = { n: 42, m: 26 }
  => { :n => 42, :m => 26 }
  >>> add(**d)
  => 68

***************
Data Structures
***************

Sets
====

A *set* is a data structure including objects with no duplicate elements.
When declaring a set, enclose elements with braces ``{`` and ``}``::

  >>> s = { 42, "foo", 3.141592 }
  => <Set 000000000000028c>
  >>> s.include?(42)
  => true
  >>> s.include?("bar")
  => false

If you make an empty set, use ``Set.new()``.
``{}`` becomes an empty dictionary (below)::

  >>> s = Set.new()
  => <Set 000000000000027d>
  >>> s.size
  => 0

Dictionaries
============

A dictionary is a data structure which is called in other languages as "hashes", "maps" or "associative array".
Dictionaries are indexed by any type's object to get a corresponding value.
When make a dictionary, enclosing key-value pairs in the form of ``key => value`` in braces ``{`` and ``}``.
A few examples are following::

  >>> dict = { 42 => 26, "foo" => "bar" }
  => { "foo" => "bar", 42 => 26 }
  >>> dict[42]
  => 26
  >>> dict["foo"]
  => bar
  >>> dict.each() do [key, value]
  ...   puts(key, value)
  ... end
  foo
  bar
  42
  26
  => { "foo" => "bar", 42 => 26 }

********
Packages
********

When you write programs for some time, you may want to reuse some useful codes.
*Packages* resolve this issue.
You can use codes in packages from external scripts.

All of Yog's scripts are packages.
If a script's file name is ``foo.yg``, its package name is ``foo``.
First of all, packages must be *imported*.
To import a package, use ``import`` statement::

  import foo

The imported package has its internal definitions as its attributes.
When the function ``bar`` is defined in the package ``foo``, ``foo.bar`` after importing the package is the ``bar`` function, so you can call this in the way of ``foo.bar()``.

Show you one example.
Suppose the package ``fib``.
This package's file name is ``fib.yg``, and ``fib.yg`` is the following::

  def fib(n)
    if (n == 0) || (n == 1)
      return 1
    else
      return fib(n - 1) + fib(n - 2)
    end
  end

Assume that you want to use the ``fib`` function in ``fib.yg`` from the script ``foo.yg``.
``foo.yg`` is following::

  import fib
  printf(fib(43))

The Package Search Path
=======================

When do ``import foo``, Yog searches ``foo.yg`` in the following directories.

# current directory
# ``PREFIX/lib/yog/0.0.1`` (``PREFIX`` is usually ``/usr/local``)

Package Hierarchy
=================

Packages are placed in the packages' tree.

Assume that you want to make a package ``foo.bar``.
In this case, first of all, you should make ``foo.yg``.
At next, make a directory named ``foo``.
At the end, create a script ``foo/bar.yg``.
Finally, the directory structure becomes the following::

  +- foo.yg
  +- foo
      ~+- bar.yg

When do ``import foo.bar``, Yog executes ``foo.yg`` at first.
After this, Yog executes ``foo/bar.yg``.

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
