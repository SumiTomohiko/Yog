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
     ELIF = 263,
     ELSE = 264,
     END = 265,
     EQUAL = 266,
     EXCEPT = 267,
     FINALLY = 268,
     IF = 269,
     LPAR = 270,
     NAME = 271,
     NEWLINE = 272,
     NEXT = 273,
     NUMBER = 274,
     PLUS = 275,
     RPAR = 276,
     TRY = 277,
     WHILE = 278
   };
#endif
/* Tokens.  */
#define AS 258
#define BREAK 259
#define COMMA 260
#define COMP_OP 261
#define DEF 262
#define ELIF 263
#define ELSE 264
#define END 265
#define EQUAL 266
#define EXCEPT 267
#define FINALLY 268
#define IF 269
#define LPAR 270
#define NAME 271
#define NEWLINE 272
#define NEXT 273
#define NUMBER 274
#define PLUS 275
#define RPAR 276
#define TRY 277
#define WHILE 278




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 193 "parser.y"
typedef union YYSTYPE {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 91 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



