/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     AS = 258,
     BREAK = 259,
     COMMA = 260,
     DEF = 261,
     ELSE = 262,
     END = 263,
     EQUAL = 264,
     EXCEPT = 265,
     FINALLY = 266,
     LPAR = 267,
     NAME = 268,
     NEWLINE = 269,
     NEXT = 270,
     NUMBER = 271,
     PLUS = 272,
     RPAR = 273,
     TRY = 274,
     WHILE = 275
   };
#endif
/* Tokens.  */
#define AS 258
#define BREAK 259
#define COMMA 260
#define DEF 261
#define ELSE 262
#define END 263
#define EQUAL 264
#define EXCEPT 265
#define FINALLY 266
#define LPAR 267
#define NAME 268
#define NEWLINE 269
#define NEXT 270
#define NUMBER 271
#define PLUS 272
#define RPAR 273
#define TRY 274
#define WHILE 275




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 125 "parser.y"
typedef union YYSTYPE {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 85 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



