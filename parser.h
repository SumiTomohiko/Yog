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
     BAR = 260,
     BREAK = 261,
     CLASS = 262,
     COMMA = 263,
     COMP_OP = 264,
     DEF = 265,
     DO = 266,
     DOT = 267,
     DOUBLE_STAR = 268,
     ELIF = 269,
     ELSE = 270,
     END = 271,
     EQUAL = 272,
     EXCEPT = 273,
     FINALLY = 274,
     GREATER = 275,
     IF = 276,
     LBRACE = 277,
     LBRACKET = 278,
     LPAR = 279,
     NAME = 280,
     NEWLINE = 281,
     NEXT = 282,
     NUMBER = 283,
     PLUS = 284,
     RBRACE = 285,
     RBRACKET = 286,
     RPAR = 287,
     STAR = 288,
     TRY = 289,
     WHILE = 290
   };
#endif
/* Tokens.  */
#define AMPER 258
#define AS 259
#define BAR 260
#define BREAK 261
#define CLASS 262
#define COMMA 263
#define COMP_OP 264
#define DEF 265
#define DO 266
#define DOT 267
#define DOUBLE_STAR 268
#define ELIF 269
#define ELSE 270
#define END 271
#define EQUAL 272
#define EXCEPT 273
#define FINALLY 274
#define GREATER 275
#define IF 276
#define LBRACE 277
#define LBRACKET 278
#define LPAR 279
#define NAME 280
#define NEWLINE 281
#define NEXT 282
#define NUMBER 283
#define PLUS 284
#define RBRACE 285
#define RBRACKET 286
#define RPAR 287
#define STAR 288
#define TRY 289
#define WHILE 290




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 256 "parser.y"
typedef union YYSTYPE {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 115 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



