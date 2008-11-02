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
     AMPER = 258,
     AS = 259,
     BREAK = 260,
     COMMA = 261,
     COMP_OP = 262,
     DEF = 263,
     DOUBLE_STAR = 264,
     ELIF = 265,
     ELSE = 266,
     END = 267,
     EQUAL = 268,
     EXCEPT = 269,
     FINALLY = 270,
     IF = 271,
     LPAR = 272,
     NAME = 273,
     NEWLINE = 274,
     NEXT = 275,
     NUMBER = 276,
     PLUS = 277,
     RPAR = 278,
     STAR = 279,
     TRY = 280,
     WHILE = 281
   };
#endif
/* Tokens.  */
#define AMPER 258
#define AS 259
#define BREAK 260
#define COMMA 261
#define COMP_OP 262
#define DEF 263
#define DOUBLE_STAR 264
#define ELIF 265
#define ELSE 266
#define END 267
#define EQUAL 268
#define EXCEPT 269
#define FINALLY 270
#define IF 271
#define LPAR 272
#define NAME 273
#define NEWLINE 274
#define NEXT 275
#define NUMBER 276
#define PLUS 277
#define RPAR 278
#define STAR 279
#define TRY 280
#define WHILE 281




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 242 "parser.y"
typedef union YYSTYPE {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 97 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



