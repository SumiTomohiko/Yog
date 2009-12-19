
:mod:`builtins` --- Built-in module
====================================

.. module:: builtins
   :synopsis: The module that provides the built-in namespace.

Built-in Functions
------------------

.. function:: classmethod(function)

   :arg function: function
   :return: class method for *function*

   Return a class method for *function*.
   To declare class method, use this function in the decorator form::

      class Foo
        @classmethod
        def bar()
          # ...
        end
      end

.. function:: get_current_thread()

   :return: current thread

   Return the current thread.

   The Yog interpreter uses this function internally.

.. function:: import_package(name)

   :arg name: symbol of package name
   :return: imported package

   Import a package.

   The Yog interpreter uses this function internally.
   If you want to import packages, use :keyword:`import` statement.

.. function:: include_module(klass, module)

   :arg klass: class to include *module*
   :arg module: included module
   :return: *klass*

   Mix-in *module* to *klass*.

   The Yog interpreter uses this function internally.
   If you want to mix-in a class and modules, use :func:`include` function as a decorator.

.. function:: print(\*args)

   :arg args: objects to print standard output
   :return: :keyword:`nil`

   Print *args* to the standard output.
   If objects are not string, they are converted with :meth:`Object#to_s`.
   When no object are given, :func:`print` does nothing.

.. function:: property(getter, setter=nil)

   :arg getter: a function called on getting property
   :arg setter: a function called on setting property
   :return: a :class:`Property` object

   When a :class:`Property` object is gotten as an attribute, *getter* is called without arguments.
   If an attirubte to be set is a :class:`Property` object, *setter* is called with one argment.
   The *setter*'s argument is a new value of the attribute::

      class Foo
        def init()
          self.baz = 42
        end

        def get_bar()
          return self.baz
        end

        def set_bar(baz)
          self.baz = baz
        end

        bar = property(get_bar, set_bar)
      end

      foo = Foo.new()
      foo.bar # => 42
      foo.bar = 26
      foo.bar # => 26

.. function:: puts(\*args)

   :arg args: objects to print standard output
   :return: :keyword:`nil`

   Print *args* to the standard output with trailing newlines.
   If objects are not string, they are converted with :meth:`Object#to_s`.
   When no object are given, :func:`print` prints one newline.

.. function:: raise_exception(e)

   :arg e: an exception
   :return: :keyword:`nil`. But this function never return!

   Raise an exception.

   The Yog interpreter uses this function internally.
   If you want to raise an exception, use the :keyword:`raise` statement.

.. vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
