
CMD = ./blow

.PHONY: build

all: build

build:
	@$(CMD) build

clean:
	@$(CMD) clean

install:
	@$(CMD) install

install-dev:
	@$(CMD) install-dev

deinstall:
	@$(CMD) deinstall

deinstall-dev:
	@$(CMD) deinstall-dev

test:
	@-mv tests.log tests.log.old
	@./run_tests 2>&1 | tee tests.log
