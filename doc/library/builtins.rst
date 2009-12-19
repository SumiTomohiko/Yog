
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

.. function:: import_package(name)

   :arg name: symbol of package name
   :return: imported package

   Import a package.

.. function:: print(\*args)

   :arg args: objects to print standard output
   :return: ``nil``

   Print *args* to the standard output.
   If objects are not string, they are converted with :meth:`Object#to_s`.
   When no object are given, :func:`print` does nothing.

.. function:: puts(\*args)

   :arg args: objects to print standard output
   :return: ``nil``

   Print *args* to the standard output with trailing newlines.
   If objects are not string, they are converted with :meth:`Object#to_s`.
   When no object are given, :func:`print` prints one newline.

.. vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
