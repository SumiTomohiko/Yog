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
     GREATER = 274,
     IF = 275,
     LBRACE = 276,
     LBRACKET = 277,
     LESS = 278,
     LPAR = 279,
     NAME = 280,
     NEWLINE = 281,
     NEXT = 282,
     NUMBER = 283,
     PLUS = 284,
     RBRACE = 285,
     RBRACKET = 286,
     RETURN = 287,
     RPAR = 288,
     STAR = 289,
     TRY = 290,
     WHILE = 291,
     t__LINE__ = 292
   };
#endif
/* Tokens.  */
#define AMPER 258
#define AS 259
#define BAR 260
#define BREAK 261
#define CLASS 262
#define COMMA 263
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
#define GREATER 274
#define IF 275
#define LBRACE 276
#define LBRACKET 277
#define LESS 278
#define LPAR 279
#define NAME 280
#define NEWLINE 281
#define NEXT 282
#define NUMBER 283
#define PLUS 284
#define RBRACE 285
#define RBRACKET 286
#define RETURN 287
#define RPAR 288
#define STAR 289
#define TRY 290
#define WHILE 291
#define t__LINE__ 292




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 188 "parser.y"
typedef union YYSTYPE {
    struct YogArray* array;
    struct YogNode* node;
    struct YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 119 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



