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
     tAMPER = 258,
     tAS = 259,
     tBAR = 260,
     tBREAK = 261,
     tCLASS = 262,
     tCOMMA = 263,
     tDEF = 264,
     tDIV = 265,
     tDO = 266,
     tDOT = 267,
     tDOUBLE_STAR = 268,
     tELIF = 269,
     tELSE = 270,
     tEND = 271,
     tEQUAL = 272,
     tEQUAL_TILDA = 273,
     tEXCEPT = 274,
     tFINALLY = 275,
     tGREATER = 276,
     tIF = 277,
     tLBRACE = 278,
     tLBRACKET = 279,
     tLESS = 280,
     tLPAR = 281,
     tLSHIFT = 282,
     tNAME = 283,
     tNEWLINE = 284,
     tNEXT = 285,
     tNONLOCAL = 286,
     tNUMBER = 287,
     tPLUS = 288,
     tRBRACE = 289,
     tRBRACKET = 290,
     tREGEXP = 291,
     tRETURN = 292,
     tRPAR = 293,
     tSTAR = 294,
     tSTRING = 295,
     tTRY = 296,
     tWHILE = 297,
     tFALSE = 298,
     tTRUE = 299,
     t__LINE__ = 300
   };
#endif
/* Tokens.  */
#define tAMPER 258
#define tAS 259
#define tBAR 260
#define tBREAK 261
#define tCLASS 262
#define tCOMMA 263
#define tDEF 264
#define tDIV 265
#define tDO 266
#define tDOT 267
#define tDOUBLE_STAR 268
#define tELIF 269
#define tELSE 270
#define tEND 271
#define tEQUAL 272
#define tEQUAL_TILDA 273
#define tEXCEPT 274
#define tFINALLY 275
#define tGREATER 276
#define tIF 277
#define tLBRACE 278
#define tLBRACKET 279
#define tLESS 280
#define tLPAR 281
#define tLSHIFT 282
#define tNAME 283
#define tNEWLINE 284
#define tNEXT 285
#define tNONLOCAL 286
#define tNUMBER 287
#define tPLUS 288
#define tRBRACE 289
#define tRBRACKET 290
#define tREGEXP 291
#define tRETURN 292
#define tRPAR 293
#define tSTAR 294
#define tSTRING 295
#define tTRY 296
#define tWHILE 297
#define tFALSE 298
#define tTRUE 299
#define t__LINE__ 300




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 326 "parser.y"
typedef union YYSTYPE {
    struct YogVal val;
    ID name;
    unsigned int lineno;
} YYSTYPE;
/* Line 1403 of yacc.c.  */
#line 134 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



