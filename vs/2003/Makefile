
all:
	cd gmp
	$(MAKE) /$(MAKEFLAGS) /NOLOGO /Fgmp.mak
	cd ..
	cd onig
	$(MAKE) /$(MAKEFLAGS) /NOLOGO
	cd ..
	cd pthreads-w32
	$(MAKE) /$(MAKEFLAGS) /NOLOGO VC-static-debug
	cd ..
	cd yog
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
	cd yog
	$(MAKE) /$(MAKEFLAGS) /NOLOGO clean
	cd ..

test:
	cd ..\..\tests
	$(MAKE) /$(MAKEFLAGS) /NOLOGO /FMakefile.vs

# vim: tabstop=8 shiftwidth=8 noexpandtab
