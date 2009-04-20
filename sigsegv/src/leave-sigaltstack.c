/* Leaving a signal handler executing on the alternate stack.
   Copyright (C) 2002-2003  Bruno Haible <bruno@clisp.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <stddef.h>
#include <signal.h>
#if HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif

/* For MacOSX.  */
#ifndef SS_ONSTACK
#define SS_ONSTACK SA_ONSTACK
#endif

void
sigsegv_reset_onstack_flag (void)
{
  stack_t ss;

  if (sigaltstack (NULL, &ss) >= 0)
    {
      ss.ss_flags &= ~SS_ONSTACK;
      sigaltstack (&ss, NULL);
    }
}
