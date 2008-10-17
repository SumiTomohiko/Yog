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
     COMP_OP = 261,
     DEF = 262,
     ELSE = 263,
     END = 264,
     EQUAL = 265,
     EXCEPT = 266,
     FINALLY = 267,
     LPAR = 268,
     NAME = 269,
     NEWLINE = 270,
     NEXT = 271,
     NUMBER = 272,
     PLUS = 273,
     RPAR = 274,
     TRY = 275,
     WHILE = 276
   };
#endif
/* Tokens.  */
#define AS 258
#define BREAK 259
#define COMMA 260
#define COMP_OP 261
#define DEF 262
#define ELSE 263
#define END 264
#define EQUAL 265
#define EXCEPT 266
#define FINALLY 267
#define LPAR 268
#define NAME 269
#define NEWLINE 270
#define NEXT 271
#define NUMBER 272
#define PLUS 273
#define RPAR 274
#define TRY 275
#define WHILE 276




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 185 "parser.y"
typedef union YYSTYPE {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 87 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



