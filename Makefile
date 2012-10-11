
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
