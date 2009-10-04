# Makefile for Microsoft Visual Studio

#LIBS = test_package4.dll test_package5.dll test_package6.dll
LIBS =
CFLAGS = $(CFLAGS) /MD /O2 /Wall /nologo /QI0f /Z7 /I..\include \
	 /I..\vs\2003\Yog /link /DLL

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
