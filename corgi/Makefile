
WAF = python3 ./waf -v

all: build

configure:
	@$(WAF) configure

install:
	@$(WAF) install

clean:
	@$(WAF) clean

build:
	@$(WAF) build

dist:
	@$(WAF) dist

distcheck:
	@$(WAF) distcheck

doc:
	if [ ! -h doc/index.rst ]; then \
		ln -s `pwd`/README.rst `pwd`/doc/index.rst; \
	fi
	cd doc && make html

.PHONY: build doc

# vim: tabstop=8 shiftwidth=8 noexpandtab
