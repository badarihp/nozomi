.PHONY: build test-all test

build:
	$(shell buck build src/...)

test-all:
	$(shell buck test --include="integration" src/...)

test:
	$(shell buck test src/...)

help:
	@echo "This library is built with buck. (buckbuild.org)"
	@echo " make help - Show this help"
	@echo " make build - Builds the library and any example binaries"
	@echo " make test - Runs basic unit tests"
	@echo " make test-all - Runs full test suite, including integration tests"

all: build
