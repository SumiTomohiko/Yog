
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

TEST_LOG = all-tests.log

test:
	@-mv $(TEST_LOG) $(TEST_LOG).old
	@./run_tests 2>&1 | tee $(TEST_LOG)
