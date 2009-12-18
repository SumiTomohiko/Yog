
:mod:`builtins` --- Built-in module
====================================

.. module:: builtins
   :synopsis: The module that provides the built-in namespace.

Built-in Functions
------------------

.. function:: print(*args)

   Print *args* to the standard output.
   If objects are not string, they are converted with :meth:`Object#to_s`.
   When no object are given, :func:`print` does nothing.

.. function:: puts(*args)

   Print *args* to the standard output with trailing newlines.
   If objects are not string, they are converted with :meth:`Object#to_s`.
   When no object are given, :func:`print` prints one newline.

.. vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
