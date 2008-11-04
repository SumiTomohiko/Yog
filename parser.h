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
     DO = 264,
     DOUBLE_STAR = 265,
     ELIF = 266,
     ELSE = 267,
     END = 268,
     EQUAL = 269,
     EXCEPT = 270,
     FINALLY = 271,
     IF = 272,
     LBRACKET = 273,
     LPAR = 274,
     NAME = 275,
     NEWLINE = 276,
     NEXT = 277,
     NUMBER = 278,
     PLUS = 279,
     RBRACKET = 280,
     RPAR = 281,
     STAR = 282,
     TRY = 283,
     WHILE = 284
   };
#endif
/* Tokens.  */
#define AMPER 258
#define AS 259
#define BREAK 260
#define COMMA 261
#define COMP_OP 262
#define DEF 263
#define DO 264
#define DOUBLE_STAR 265
#define ELIF 266
#define ELSE 267
#define END 268
#define EQUAL 269
#define EXCEPT 270
#define FINALLY 271
#define IF 272
#define LBRACKET 273
#define LPAR 274
#define NAME 275
#define NEWLINE 276
#define NEXT 277
#define NUMBER 278
#define PLUS 279
#define RBRACKET 280
#define RPAR 281
#define STAR 282
#define TRY 283
#define WHILE 284




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 251 "parser.y"
typedef union YYSTYPE {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 103 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



