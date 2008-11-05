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
     COMMA = 262,
     COMP_OP = 263,
     DEF = 264,
     DO = 265,
     DOT = 266,
     DOUBLE_STAR = 267,
     ELIF = 268,
     ELSE = 269,
     END = 270,
     EQUAL = 271,
     EXCEPT = 272,
     FINALLY = 273,
     IF = 274,
     LBRACE = 275,
     LBRACKET = 276,
     LPAR = 277,
     NAME = 278,
     NEWLINE = 279,
     NEXT = 280,
     NUMBER = 281,
     PLUS = 282,
     RBRACE = 283,
     RBRACKET = 284,
     RPAR = 285,
     STAR = 286,
     TRY = 287,
     WHILE = 288
   };
#endif
/* Tokens.  */
#define AMPER 258
#define AS 259
#define BAR 260
#define BREAK 261
#define COMMA 262
#define COMP_OP 263
#define DEF 264
#define DO 265
#define DOT 266
#define DOUBLE_STAR 267
#define ELIF 268
#define ELSE 269
#define END 270
#define EQUAL 271
#define EXCEPT 272
#define FINALLY 273
#define IF 274
#define LBRACE 275
#define LBRACKET 276
#define LPAR 277
#define NAME 278
#define NEWLINE 279
#define NEXT 280
#define NUMBER 281
#define PLUS 282
#define RBRACE 283
#define RBRACKET 284
#define RPAR 285
#define STAR 286
#define TRY 287
#define WHILE 288




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 256 "parser.y"
typedef union YYSTYPE {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 111 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



