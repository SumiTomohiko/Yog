
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
	$(MAKE) /$(MAKEFLAGS) /NOLOGO /FMakefile.vs

installer: Yog-0.0.3.msi

Yog-0.0.3.msi: Yog-0.0.3.wixobj
	light -nologo Yog-0.0.3.wixobj

Yog-0.0.3.wixobj: Yog-0.0.3.wxs
	candle -nologo Yog-0.0.3.wxs

# vim: tabstop=8 shiftwidth=8 noexpandtab
