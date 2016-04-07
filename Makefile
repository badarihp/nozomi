build:
	$(shell buck build src/...)

run:
	$(shell buck run src:requests-bin http://google.com)

test-full:
    $(shell buck test --include="integration" src/...)

test:
    $(shell buck test src/...)

install:
    $(shell buck install src/requests-bin)

help:
	@echo "Help!"

all: build
