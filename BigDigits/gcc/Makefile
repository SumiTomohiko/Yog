# Linux/Cygwin gcc Makefile for bigdigits tests
# $Date: 2008-07-31 16:24:00 $
# $Revision: 2.2.0 $
# $Author: dai $
#

OBJECTS= bigd.o bigdigits.o bigdigitsRand.o bigdRand.o
INCLUDES= bigd.h bigdigits.h bigdtypes.h bigdRand.h bigdigitsRand.h
CFLAGS= -std=c99 -pedantic -Wall -Wpointer-arith -Wstrict-prototypes -O2
CC= gcc

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $<
	
# bigd "bd" tests

t_bdTest: t_bdTest.o $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)

t_bdSimple: t_bdSimple.o $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)

t_bdRSA: t_bdRSA.o $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)

t_bdRSA1: t_bdRSA1.o $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)

t_bdDSA: t_bdDSA.o $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)

t_bdRDSA: t_bdRDSA.o $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)


# bigdigits "mp" tests

t_mpTest: t_mpTest.o $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)

t_mpRSA508: t_mpRSA508.o $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)

t_mpJacobi: t_mpJacobi.o $(INCLUDES) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)


# extra "sp" function tests

t_spExtras: t_spExtras.o spExtras.o
	$(CC) $(CFLAGS) -o $@ $@.o spExtras.o $(OBJECTS) $(LDFLAGS)

spExtras.o: spExtras.c spExtras.h $(INCLUDES)

# Call functions from a C++ program
t_bdCPP: t_bdCPP.o $(INCLUDES) $(OBJECTS)
	g++ $(CFLAGS) -o $@ $@.o $(OBJECTS) $(LDFLAGS)

clean:
	-rm -f *.o
