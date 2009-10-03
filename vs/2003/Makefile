
all:
	cd gmp
	$(MAKE) /$(MAKEFLAGS) /NOLOGO /Fgmp.mak
	cd ..
	cd onig
	$(MAKE) /$(MAKEFLAGS) /NOLOGO
	cd ..
	cd pthreads-w32
	$(MAKE) /$(MAKEFLAGS) /NOLOGO
	cd ..
	cd Yog
	$(MAKE) /$(MAKEFLAGS) /NOLOGO
	cd ..

clean:
	cd gmp
	$(MAKE) /$(MAKEFLAGS) /NOLOGO /Fgmp.mak clean
	cd ..
	cd onig
	$(MAKE) /$(MAKEFLAGS) /NOLOGO clean
	cd ..
	cd pthreads-w32
	$(MAKE) /$(MAKEFLAGS) /NOLOGO clean
	cd ..
	cd Yog
	$(MAKE) /$(MAKEFLAGS) /NOLOGO clean
	cd ..

test:
	cd ..\..\tests
	py.test --showlocals

# vim: tabstop=8 shiftwidth=8 noexpandtab
