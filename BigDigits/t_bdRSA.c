/* $Id: t_bdRSA.c $ */

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

/* Test BigDigits "bd" functions using a new RSA key and random data */

#if _MSC_VER >= 1100
	/* Detect memory leaks in MSVC++ */ 
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#else
	#include <stdlib.h>
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "bigd.h"

static int my_rand(unsigned char *bytes, size_t nbytes, const unsigned char *seed, size_t seedlen)
/* Our own (very insecure) random generator func using good old rand() 
   but in the required format for BD_RANDFUNC
   -- replace this in practice with your own cryptographically-secure function.
*/
{
	unsigned int myseed;
	size_t i;
	int offset;

	/* Use time for 32-bit seed - then blend in user-supplied seed, if any */
	myseed = (unsigned)time(NULL) ^ (unsigned)clock();
	if (seed)
	{
		for (offset = 0, i = 0; i < seedlen; i++, offset = (offset + 1) % sizeof(unsigned))
			myseed ^= ((unsigned int)seed[i] << (offset * 8));
	}

	srand(myseed);
	while (nbytes--)
	{
		*bytes++ = rand() & 0xFF;
	}

	return 0;
}


static void pr_msg(char *msg, BIGD b)
{
	printf("%s", msg);
	bdPrint(b, 1);
}

#define give_a_sign(c) putchar((c))

static bdigit_t SMALL_PRIMES[] = {
	3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 
	47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 
	103, 107, 109, 113,
	127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
	179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
	233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
	283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
	353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
	419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
	467, 479, 487, 491, 499, 503, 509, 521, 523, 541,
	547, 557, 563, 569, 571, 577, 587, 593, 599, 601,
	607, 613, 617, 619, 631, 641, 643, 647, 653, 659,
	661, 673, 677, 683, 691, 701, 709, 719, 727, 733,
	739, 743, 751, 757, 761, 769, 773, 787, 797, 809,
	811, 821, 823, 827, 829, 839, 853, 857, 859, 863,
	877, 881, 883, 887, 907, 911, 919, 929, 937, 941,
	947, 953, 967, 971, 977, 983, 991, 997,
};
#define N_SMALL_PRIMES (sizeof(SMALL_PRIMES)/sizeof(bdigit_t))

int generateRSAPrime(BIGD p, size_t nbits, bdigit_t e, size_t ntests, 
				 const unsigned char *seed, size_t seedlen, BD_RANDFUNC randFunc)
/* Create a prime p such that gcd(p-1, e) = 1.
   Returns # prime tests carried out or -1 if failed.
   Sets the TWO highest bits to ensure that the 
   product pq will always have its high bit set.
   e MUST be a prime > 2.
   This function assumes that e is prime so we can
   do the less expensive test p mod e != 1 instead
   of gcd(p-1, e) == 1.
   Uses improvement in trial division from Menezes 4.51.
  */
{
	BIGD u;
	size_t i, j, iloop, maxloops, maxodd;
	int done, overflow, failedtrial;
	int count = 0;
	bdigit_t r[N_SMALL_PRIMES];

	/* Create a temp */
	u = bdNew();

	maxodd = nbits * 100;
	maxloops = 5;

	done = 0;
	for (iloop = 0; !done && iloop < maxloops; iloop++)
	{
		/* Set candidate n0 as random odd number */
		bdRandomSeeded(p, nbits, seed, seedlen, randFunc);
		/* Set two highest and low bits */
		bdSetBit(p, nbits - 1, 1);
		bdSetBit(p, nbits - 2, 1);
		bdSetBit(p, 0, 1);

		/* To improve trial division, compute table R[q] = n0 mod q
		   for each odd prime q <= B
		*/
		for (i = 0; i < N_SMALL_PRIMES; i++)
		{
			r[i] = bdShortMod(u, p, SMALL_PRIMES[i]);
		}

		done = overflow = 0;
		/* Try every odd number n0, n0+2, n0+4,... until we succeed */
		for (j = 0; j < maxodd; j++, overflow = bdShortAdd(p, p, 2))
		{
			/* Check for overflow */
			if (overflow)
				break;

			give_a_sign('.');
			count++;

			/* Each time 2 is added to the current candidate
			   update table R[q] = (R[q] + 2) mod q */
			if (j > 0)
			{
				for (i = 0; i < N_SMALL_PRIMES; i++)
				{
					r[i] = (r[i] + 2) % SMALL_PRIMES[i];
				}
			}

			/* Candidate passes the trial division stage if and only if
			   NONE of the R[q] values equal zero */
			for (failedtrial = 0, i = 0; i < N_SMALL_PRIMES; i++)
			{
				if (r[i] == 0)
				{
					failedtrial = 1;
					break;
				}
			}
			if (failedtrial)
				continue;

			/* If p mod e = 1 then gcd(p, e) > 1, so try again */
			bdShortMod(u, p, e);
			if (bdShortCmp(u, 1) == 0)
				continue;

			/* Do expensive primality test */
			give_a_sign('*');
			if (bdRabinMiller(p, ntests))
			{	/* Success! - we have a prime */
				done = 1;
				break;
			}

		}
	}
	

	/* Clear up */
	bdFree(&u);
	printf("\n");

	return (done ? count : -1);
}

int generateRSAKey(BIGD n, BIGD e, BIGD d, BIGD p, BIGD q, BIGD dP, BIGD dQ, BIGD qInv,
	size_t nbits, bdigit_t ee, size_t ntests, unsigned char *seed, size_t seedlen, 
	BD_RANDFUNC randFunc)
{
	BIGD g, p1, q1, phi;
	size_t np, nq;
	unsigned char *myseed = NULL;
	clock_t start, finish;
	double duration, tmake;
	long ptests;
	int res;

	/* Initialise */
	g = bdNew();
	p1 = bdNew();
	q1 = bdNew();
	phi = bdNew();

	printf("Generating a %d-bit RSA key...\n", nbits);
	
	/* We add an extra byte to the user-supplied seed */
	myseed = malloc(seedlen + 1);
	if (!myseed) return -1;
	memcpy(myseed, seed, seedlen);

	/* Do (p, q) in two halves, approx equal */
	nq = nbits / 2 ;
	np = nbits - nq;

	/* Make sure seeds are slightly different for p and q */
	myseed[seedlen] = 0x01;
	start = clock();
	res = generateRSAPrime(p, np, ee, ntests, myseed, seedlen+1, randFunc);
	finish = clock();
	pr_msg("p=", p);
	assert(res > 0);
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	printf("generateRSAPrime took %.3f secs and %d prime candidates (%.4f s/test)\n", duration, res, duration / res); 
	ptests = res;
	tmake = duration;
	printf("p is %d bits\n", bdBitLength(p));

	myseed[seedlen] = 0xff;
	start = clock();
	res = generateRSAPrime(q, nq, ee, ntests, myseed, seedlen+1, randFunc);
	finish = clock();
	pr_msg("q=", q);
	assert(res > 0);
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	printf("generateRSAPrime took %.3f secs and %d prime candidates (%.4f s/test)\n", duration, res, duration / res); 
	ptests += res;
	tmake += duration;
	printf("q is %d bits\n", bdBitLength(q));
	/* Check that p != q (if so, RNG is faulty!) */
	assert(!bdIsEqual(p, q));

	bdSetShort(e, ee);
	pr_msg("e=", e);

	/* If q > p swap p and q so p > q */
	if (bdCompare(p, q) < 1)
	{	
		bdSetEqual(g, p);
		bdSetEqual(p, q);
		bdSetEqual(q, g);
	}

	/* Calc p-1 and q-1 */
	bdSetEqual(p1, p);
	bdDecrement(p1);
	pr_msg("p-1=\n", p1);
	bdSetEqual(q1, q);
	bdDecrement(q1);
	pr_msg("q-1=\n", q1);

	/* Check gcd(p-1, e) = 1 */
	bdGcd(g, p1, e);
	pr_msg("gcd(p-1,e)=", g);
	assert(bdShortCmp(g, 1) == 0);
	bdGcd(g, q1, e);
	pr_msg("gcd(q-1,e)=", g);
	assert(bdShortCmp(g, 1) == 0);

	/* Compute n = pq */
	bdMultiply(n, p, q);
	pr_msg("n=\n", n);

	/* Compute d = e^-1 mod (p-1)(q-1) */
	bdMultiply(phi, p1, q1);
	pr_msg("phi=\n", phi);
	res = bdModInv(d, e, phi);
	assert(res == 0);
	pr_msg("d=\n", d);

	/* Check ed = 1 mod phi */
	bdModMult(g, e, d, phi);
	pr_msg("ed mod phi=", g);
	assert(bdShortCmp(g, 1) == 0);

	/* Calculate CRT key values */
	printf("CRT values:\n");
	bdModInv(dP, e, p1);
	bdModInv(dQ, e, q1);
	bdModInv(qInv, q, p);
	pr_msg("dP=", dP);
	pr_msg("dQ=", dQ);
	pr_msg("qInv=", qInv);

	printf("\nTime to create key = %.3f secs with %ld prime candidates (%.4f s/test)\n\n", tmake, ptests, tmake / ptests);
	printf("n is %d bits\n", bdBitLength(n));

	/* Clean up */
	if (myseed) free(myseed);
	bdFree(&g);
	bdFree(&p1);
	bdFree(&q1);
	bdFree(&phi);

	return 0;
}

static int debug = 0;

int main(void)
{
	size_t nbits = 1025;
	unsigned ee = 0x3;
	size_t ntests = 50;
	unsigned char *seed = NULL;
	size_t seedlen = 0;

	BIGD n, e, d, p, q, dP, dQ, qInv;
	BIGD m, c, s, hq, h, m1, m2; 
	int res;
	clock_t start, finish;
	double tinv, tcrt;

/* MSVC memory leak checking stuff */
#if _MSC_VER >= 1100
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
#endif

	printf("Test BIGDIGITS with a new %d-bit RSA key and random data.\n", nbits);

	/* Initialise */
	p = bdNew();
	q = bdNew();
	n = bdNew();
	e = bdNew();
	d = bdNew();
	dP= bdNew();
	dQ= bdNew();
	qInv= bdNew();
	m = bdNew();
	c = bdNew();
	s = bdNew();
	m1 = bdNew();
	m2 = bdNew();
	h = bdNew();
	hq = bdNew();

	/* Create RSA key pair (n, e),(d, p, q, dP, dQ, qInv) */
	/* NB we use simple my_rand() here -- you should use a proper cryptographically-secure RNG */
	res = generateRSAKey(n, e, d, p, q, dP, dQ, qInv, nbits, ee, ntests, seed, seedlen, my_rand);
	
	if (res != 0)
	{
		printf("Failed to generate RSA key!\n");
		goto clean_up;
	}

	/* Set a random message m < n */
	bdRandomSeeded(m, bdBitLength(n)-1, NULL, 0, my_rand);
	pr_msg("m=\n", m);

	/* Encrypt c = m^e mod n */
	bdModExp(c, m, e, n);
	pr_msg("c=\n", c);

	/* Check decrypt m1 = c^d mod n */
	start = clock();
	bdModExp(m1, c, d, n);
	finish = clock();
	tinv = (double)(finish - start) / CLOCKS_PER_SEC;
	pr_msg("m'=\n", m1);
	res = bdCompare(m1, m);
	printf("Decryption %s\n", (res == 0 ? "OK" : "FAILED!"));
	assert(res == 0);
	printf("Decrypt by inversion took %.3f secs\n", tinv);

	/* Sign s = m^d mod n */
	bdModExp(s, m, d, n);
	pr_msg("s=\n", s);

	/* Check verify m1 = s^e mod n */
	bdModExp(m1, s, e, n);
	pr_msg("m'=\n", m1);
	res = bdCompare(m1, m);
	printf("Verification %s\n", (res == 0 ? "OK" : "FAILED!"));
	assert(res == 0);

	/* Decrypt using CRT method - Ref: PKCS #1 */
	pr_msg("m=", m);
	pr_msg("c=", c);
	pr_msg("p=", p);
	pr_msg("q=", q);
	start = clock();
	/* Let m_1 = c^dP mod p. */
	bdModExp(m1, c, dP, p);
	if(debug)pr_msg("m_1=c^dP mod p=", m1);
	/* Let m_2 = c^dQ mod q. */
	bdModExp(m2, c, dQ, q);
	if(debug)pr_msg("m_2=c^dQ mod q=", m2);
	if (bdCompare(m1, m2) < 0)
		bdAdd(m1, m1, p);
	bdSubtract(m1, m1, m2);
	if(debug)pr_msg("m_1 - m_2=", m1);
	/* Let h = qInv ( m_1 - m_2 ) mod p. */
	bdModMult(h, qInv, m1, p);
	if(debug)pr_msg("h=qInv(m1-m2) mod p=", h);
	bdMultiply(hq, h, q);
	if(debug)pr_msg("hq=", hq);
	/* Let m = m_2 + hq. */
	bdAdd(m1, m2, hq);
	finish = clock();
	tcrt = (double)(finish - start) / CLOCKS_PER_SEC;
	if(debug)pr_msg("m'=m_2 + hq=", m1);
	pr_msg("(CRT)m'=\n", m1);
	res = bdCompare(m1, m);
	printf("CRT Decryption %s\n", (res == 0 ? "OK" : "FAILED!"));
	assert(res == 0);
	printf("Decrypt by CRT took %.3f secs\n", tcrt);
	printf("c.f. Decrypt by inversion %.3f secs (factor = %.1f)\n", 
		tinv, (tcrt ? tinv / tcrt : 0));
	printf("n is %d bits\n", bdBitLength(n));

	/* Clean up */
clean_up:
	bdFree(&n);
	bdFree(&e);
	bdFree(&d);
	bdFree(&p);
	bdFree(&q);
	bdFree(&dP);
	bdFree(&dQ);
	bdFree(&qInv);
	bdFree(&m);
	bdFree(&c);
	bdFree(&s);
	bdFree(&m1);
	bdFree(&m2);
	bdFree(&h);
	bdFree(&hq);

	printf("OK, successfully completed tests.\n");

	return 0;
}
