.PHONY: build docs test-all test

build:
	buck build src/...

docs:
	doxygen

test-all:
	buck test --include="integration" src/...

test:
	buck test src/...

help:
	@echo "This library is built with buck. (buckbuild.org)"
	@echo " make help - Show this help"
	@echo " make build - Builds the library and any example binaries"
	@echo " make test - Runs basic unit tests"
	@echo " make test-all - Runs full test suite, including integration tests"

all: build
