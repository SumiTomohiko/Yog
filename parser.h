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
     DIV = 265,
     DO = 266,
     DOT = 267,
     DOUBLE_STAR = 268,
     ELIF = 269,
     ELSE = 270,
     END = 271,
     EQUAL = 272,
     EQUAL_TILDA = 273,
     EXCEPT = 274,
     FINALLY = 275,
     GREATER = 276,
     IF = 277,
     LBRACE = 278,
     LBRACKET = 279,
     LESS = 280,
     LPAR = 281,
     LSHIFT = 282,
     NAME = 283,
     NEWLINE = 284,
     NEXT = 285,
     NUMBER = 286,
     PLUS = 287,
     RBRACE = 288,
     RBRACKET = 289,
     REGEXP = 290,
     RETURN = 291,
     RPAR = 292,
     STAR = 293,
     STRING = 294,
     TRY = 295,
     WHILE = 296,
     tFALSE = 297,
     tTRUE = 298,
     t__LINE__ = 299
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
#define DIV 265
#define DO 266
#define DOT 267
#define DOUBLE_STAR 268
#define ELIF 269
#define ELSE 270
#define END 271
#define EQUAL 272
#define EQUAL_TILDA 273
#define EXCEPT 274
#define FINALLY 275
#define GREATER 276
#define IF 277
#define LBRACE 278
#define LBRACKET 279
#define LESS 280
#define LPAR 281
#define LSHIFT 282
#define NAME 283
#define NEWLINE 284
#define NEXT 285
#define NUMBER 286
#define PLUS 287
#define RBRACE 288
#define RBRACKET 289
#define REGEXP 290
#define RETURN 291
#define RPAR 292
#define STAR 293
#define STRING 294
#define TRY 295
#define WHILE 296
#define tFALSE 297
#define tTRUE 298
#define t__LINE__ 299




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 311 "parser.y"
typedef union YYSTYPE {
    struct YogArray* array;
    struct YogNode* node;
    struct YogVal val;
    ID name;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 133 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



