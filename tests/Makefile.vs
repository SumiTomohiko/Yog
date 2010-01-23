# Makefile for Microsoft Visual Studio

ROOT_DIR = ..
VS_DIR = $(ROOT_DIR)\vs\2003
LIBS = test_package4.dll test_package5.dll test_package6.dll
CFLAGS = $(CFLAGS) /MD /O2 /Wall /nologo /QI0f /Z7 /I..\include /TP \
	 /I$(VS_DIR)\Yog\include /link $(VS_DIR)\Yog\yog.lib /DLL

all: $(LIBS)
	py.test --showlocals

test_package4.dll: test_package4.c Makefile
	$(CC) test_package4.c /Fe$@ $(CFLAGS)

test_package5.dll: test_package5.c Makefile
	$(CC) test_package5.c /Fe$@ $(CFLAGS)

test_package6.dll: test_package6.c Makefile
	$(CC) test_package6.c /Fe$@ $(CFLAGS)

clean:
	del /Q core* *.pyc *.dll

# vim: tabstop=8 shiftwidth=8 noexpandtab
