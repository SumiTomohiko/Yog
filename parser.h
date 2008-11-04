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
     DOT = 265,
     DOUBLE_STAR = 266,
     ELIF = 267,
     ELSE = 268,
     END = 269,
     EQUAL = 270,
     EXCEPT = 271,
     FINALLY = 272,
     IF = 273,
     LBRACKET = 274,
     LPAR = 275,
     NAME = 276,
     NEWLINE = 277,
     NEXT = 278,
     NUMBER = 279,
     PLUS = 280,
     RBRACKET = 281,
     RPAR = 282,
     STAR = 283,
     TRY = 284,
     WHILE = 285
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
#define DOT 265
#define DOUBLE_STAR 266
#define ELIF 267
#define ELSE 268
#define END 269
#define EQUAL 270
#define EXCEPT 271
#define FINALLY 272
#define IF 273
#define LBRACKET 274
#define LPAR 275
#define NAME 276
#define NEWLINE 277
#define NEXT 278
#define NUMBER 279
#define PLUS 280
#define RBRACKET 281
#define RPAR 282
#define STAR 283
#define TRY 284
#define WHILE 285




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 256 "parser.y"
typedef union YYSTYPE {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 105 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



