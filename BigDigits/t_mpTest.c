/* $Id: t_mpTest.c $ */

/******************** SHORT COPYRIGHT NOTICE**************************
This source code is part of the BigDigits multiple-precision
arithmetic library Version 2.3 originally written by David Ireland,
copyright (c) 2001-11 D.I. Management Services Pty Limited, all rights
reserved. It is provided "as is" with no warranties. You may use
this software under the terms of the full copyright notice
"bigdigitsCopyright.txt" that should have been included with this
library or can be obtained from <www.di-mgt.com.au/bigdigits.html>.
This notice must always be retained in any copy.
******************* END OF COPYRIGHT NOTICE***************************/
/*
	Last updated:
	$Date: 2011-11-11 11:11:11 $
	$Revision: 2.3.0 $
	$Author: dai $
*/

/* Some tests of the BigDigits "mp" functions */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include "bigdigits.h"
#include "bigdigitsRand.h"

#define TEST_LEN 32

static int debug = 0;

DIGIT_T u[TEST_LEN];
DIGIT_T v[TEST_LEN];
DIGIT_T w[TEST_LEN]; 
DIGIT_T q[TEST_LEN];
DIGIT_T r[TEST_LEN];
DIGIT_T p1[TEST_LEN];
DIGIT_T p2[TEST_LEN];
DIGIT_T p3[TEST_LEN];
DIGIT_T g[TEST_LEN];
DIGIT_T a[TEST_LEN];
DIGIT_T n[TEST_LEN];

void ClearAll(void)
{
	mpSetZero(u, TEST_LEN);
	mpSetZero(v, TEST_LEN);
	mpSetZero(w, TEST_LEN);
	mpSetZero(q, TEST_LEN);
	mpSetZero(r, TEST_LEN);
	mpSetZero(g, TEST_LEN);
	mpSetZero(p1, TEST_LEN);
	mpSetZero(p2, TEST_LEN);
	mpSetZero(p3, TEST_LEN);
	mpSetZero(a, TEST_LEN);
	mpSetZero(n, TEST_LEN);
}

static int MakeMultiplePrime(DIGIT_T p[], size_t ndigits)
{	/* Returns a random prime number of ndigits */
	/*	WARNING: This is not cryptographically secure 
		because the random number generator isn't */
	size_t i;

	for (i = 0; i < ndigits; i++)
		p[i] = spBetterRand();

	/* Make sure the highest and low bits are set */
	p[ndigits - 1] |= HIBITMASK;
	p[0] |= 0x1;

	//printf("p="); mpPrintNL(p, ndigits);

	/* Check if prime */
	while (!mpIsPrime(p, ndigits, 10))
	{
		/* Keep adding 2 until find a prime */
		mpShortAdd(p, p, 2, ndigits);

		//printf("p="); mpPrintNL(p, ndigits);
		printf(".");

		/* Check for overflow */
		if (!(p[ndigits - 1] & HIBITMASK))
			return -1;	/* Failed to find a prime */
	}

	return 0;
}

size_t MakeMultipleRandom(DIGIT_T a[], size_t ndigits)
{
	/* Make a random number of up to ndigits digits */ 
	size_t i, n, bits;
	DIGIT_T mask;

	n = (size_t)spSimpleRand(1, ndigits);
	for (i = 0; i < n; i++)
		a[i] = spBetterRand();
	for (i = n; i < ndigits; i++)
		a[i] = 0;

	/*	Zero out a random number of bits in leading digit 
		about half the time */
	bits = (size_t)spSimpleRand(0, 2*BITS_PER_DIGIT);
	if (bits != 0 && bits < BITS_PER_DIGIT)
	{
		mask = HIBITMASK;
		for (i = 1; i < bits; i++)
		{
			mask |= (mask >> 1);
		}
		mask = ~mask;
		a[n-1] &= mask;
	}
	return n;
}

void ShowAdd(DIGIT_T w[], DIGIT_T u[], DIGIT_T v[], 
			 DIGIT_T carry, size_t ndigits)
{
	printf("mpAdd: ");
	mpPrintTrim(u, ndigits);
	printf("+ ");
	mpPrintTrim(v, ndigits);
	printf("= ");
	mpPrintTrim(w, ndigits);
	printf(", Carry = %" PRIxBIGD "\n", carry);
}

void ShowMult(DIGIT_T w[], DIGIT_T u[], DIGIT_T v[], size_t ndigits)
{
	printf("mpMultiply: ");
	mpPrintTrim(u, ndigits);
	printf("x ");
	mpPrintTrim(v, ndigits);
	printf("= ");
	mpPrintTrim(w, ndigits*2);
	printf("\n");
}

void ShowDiv(DIGIT_T q[], DIGIT_T r[], DIGIT_T u[], DIGIT_T v[], size_t ndigits)
{
	printf("mpDivide: ");
	mpPrintTrim(u, ndigits);
	printf("/ ");
	mpPrintTrim(v, ndigits);
	printf("= ");
	mpPrintTrim(q, ndigits);
	printf("rem ");
	mpPrintTrim(r, ndigits);
	printf("\n");
}

int main(void)
{
	DIGIT_T carry, m;
	int jac;
	size_t len;

	/*	Force linker to include copyright notice in 
		executable object image
	*/
	copyright_notice();
		
	ClearAll();

	printf("Testing BIGDIGITS 'mp' functions.\n");

	/* Start easy: 1 + 1 = 2 */
	mpSetDigit(u, 1, TEST_LEN);	/* u = 1 */
	carry = mpAdd(w, u, u, TEST_LEN);	/* w = u + u */
	ShowAdd(w, u, u, carry, TEST_LEN);
	/* Check that w == 2 */
	assert(mpShortCmp(w, 2, TEST_LEN) == 0);

	/* ---- Add and subtract ---- */
	/* Add two random numbers w = u + v */
	MakeMultipleRandom(u, TEST_LEN-1);
	MakeMultipleRandom(v, TEST_LEN-1);
	carry = mpAdd(w, u, v, TEST_LEN);

	/* r = w - v */
	carry = mpSubtract(r, w, v, TEST_LEN);

	/* Check r == u */
	assert(mpEqual(r, u, TEST_LEN));
	printf("Add and subtract worked OK\n");
	ClearAll();

	/* ---- Multiply and divide ---- */
	/* Multiply two random numbers w = u * v */
	MakeMultipleRandom(u, TEST_LEN / 2);
	MakeMultipleRandom(v, TEST_LEN / 2);
	mpMultiply(w, u, v, TEST_LEN / 2);
	if (debug) ShowMult(w, u, v, TEST_LEN / 2);

	/* q = w / v, r = w % v */
	mpDivide(q, r, w, TEST_LEN, v, TEST_LEN / 2);
	/* Check q == u and r == 0 */
	if (debug) mpPrintTrimNL(q, TEST_LEN / 2);
	assert(mpEqual(q, u, TEST_LEN / 2));
	assert(mpIsZero(r, TEST_LEN / 2));

	ClearAll();

	mpSetDigit(a, 0, TEST_LEN);
	mpSetZero(n, TEST_LEN);
	assert(mpEqual(a, n, TEST_LEN));
	
	ClearAll();

	/* Pick two random numbers u, v */
	MakeMultipleRandom(u, TEST_LEN/2);
	MakeMultipleRandom(v, TEST_LEN/2);
	/* Divide one by the other: q = u / v, r = u % v */
	mpDivide(q, r, u, TEST_LEN/2, v, TEST_LEN/2);
	if (debug) ShowDiv(q, r, u, v, TEST_LEN/2);
	/* Check w = q * v + r == u */
	mpMultiply(g, q, v, TEST_LEN/2);
	mpAdd(w, g, r, TEST_LEN/2);
	assert(mpEqual(w, u, TEST_LEN/2));
	printf("Multiply and divide worked OK\n");

	/* ---- Greatest Common Divisor ---- */
	/* Pick 3x random primes p1, p2, p3 */
	printf("Creating 3 prime numbers (be patient):\n");
	MakeMultiplePrime(p1, TEST_LEN / 2);
	printf("(1)\n");
	MakeMultiplePrime(p2, TEST_LEN / 2);
	printf("(2)\n");
	MakeMultiplePrime(p3, TEST_LEN / 2);
	printf("(3)\n");

	/* Calculate two products from these primes */
	/* u = p1 * p2 */
	mpMultiply(u, p1, p2, TEST_LEN / 2);
	/* v = p1 * p3 */
	mpMultiply(v, p1, p3, TEST_LEN / 2);
	/* Now calculate g = gcd(u, v) */
	mpGcd(g, u, v, TEST_LEN);
	/* And check that g == p1 */
	assert(mpEqual(g, p1, TEST_LEN)); 
	printf("Greatest Common Divisor worked OK\n");

	/* ---- Modular Inverse ---- */
	/* Use previous prime as modulus, v */
	mpSetEqual(v, p1, TEST_LEN);

	/* Pick a small multiplier, m */
	m = spSimpleRand(1, MAX_DIGIT);

	/* Set u = (vm - 1) */
	mpShortMult(u, v, m, TEST_LEN);
	mpShortSub(u, u, 1, TEST_LEN);

	mpModInv(w, u, v, TEST_LEN);
	/* Check that g = (w * u) mod v = 1 */
	mpModMult(g, w, u, v, TEST_LEN);
	assert((mpShortCmp(g, 1, TEST_LEN) == 0));
	printf("Modular inverse worked OK\n");

	/* Compute some Jacobi and Legendre symbol values */
	mpSetDigit(a, 158, TEST_LEN);
	mpSetDigit(n, 235, TEST_LEN);
	jac = mpJacobi(a, n, TEST_LEN);
	//printf("Jacobi(158, 235)=%d\n", jac);
	assert(-1 == jac);
	mpSetDigit(a, 2183, TEST_LEN);
	mpSetDigit(n, 9907, TEST_LEN);
	jac = mpJacobi(a, n, TEST_LEN);
	//printf("Jacobi(2183, 9907)=%d\n", jac);
	assert(1 == jac);
	mpSetDigit(a, 1001, TEST_LEN);
	jac = mpJacobi(a, n, TEST_LEN);
	//printf("Jacobi(1001, 9907)=%d\n", jac);
	assert(-1 == jac);
	mpShortMult(a, n, 10000, TEST_LEN);
	jac = mpJacobi(a, n, TEST_LEN);
	//printf("Jacobi(10000 * 9907, 9907)=%d\n", jac);
	assert(0 == jac);
	printf("Jacobi symbol tests worked OK\n");

	/* ---- Square, square root and cube root ---- */
	printf("\nSquare roots, etc...\n");
	/* Pick a random number u */
	MakeMultipleRandom(u, TEST_LEN/2);
	mpPrintHex("u=", u, TEST_LEN/2, "\n");
	/* Compute square */
	mpSquare(v, u, TEST_LEN/2);
	mpPrintHex("u^2=", v, TEST_LEN, "\n");
	/* Compute square root */
	mpSqrt(r, v, TEST_LEN);
	mpPrintHex("sqrt(u^2)=", r, TEST_LEN, "\n");
	/* Now compute square root of v - 1 */
	mpShortSub(v, v, 1, TEST_LEN);
	mpSqrt(w, v, TEST_LEN);
	mpPrintHex("sqrt(u^2-1)=", w, TEST_LEN, "\n");
	/* This should be one less than before */
	mpSubtract(g, r, w, TEST_LEN);
	mpPrintHex("difference=", g, TEST_LEN, " (expecting 1)\n");
	assert(mpShortCmp(g, 1, TEST_LEN) == 0);

	/* Compute cube */
	ClearAll();
	/* Pick another random number u */
	len = TEST_LEN/4;
	MakeMultipleRandom(u, len);
	mpPrintHex("u=", u, len, "\n");
	/* Compute cube (NB we use LEN/4 to avoid overflow on 2nd multiplication) */
	mpSquare(w, u, len);
	len = 2 * len;
	mpMultiply(v, w, u, len);
	mpPrintHex("u^3=", v, TEST_LEN, "\n");
	/* Compute cube root */
	mpCubeRoot(r, v, TEST_LEN);
	mpPrintHex("cuberoot(u^3)=", r, TEST_LEN, "\n");
	assert(mpCompare(r, u, TEST_LEN) == 0);

	/* Now compute cube root of v - 1 */
	mpShortSub(v, v, 1, TEST_LEN);
	mpCubeRoot(w, v, TEST_LEN);
	mpPrintHex("cuberoot(u^3-1)=", w, TEST_LEN, "\n");
	/* This should be one less than before */
	mpSubtract(g, r, w, TEST_LEN);
	mpPrintHex("difference=", g, TEST_LEN, " (expecting 1)\n");
	assert(mpShortCmp(g, 1, TEST_LEN) == 0);

	/* Display version number */
	printf("\nVersion=%d\n", mpVersion());

	/* For further checks do RSA calculation - see t_mpRSA.c */
	printf("OK, successfully completed tests.\n");
	
	return 0;
}

