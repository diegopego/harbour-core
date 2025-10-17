ROOT        := ../..
HB_PLATFORM ?= linux
HB_COMPILER ?= gcc

CC      ?= gcc
CFLAGS  ?=
LDFLAGS ?=

INCLUDES := -I$(ROOT)/include
LIBDIR   := $(ROOT)/lib/$(HB_PLATFORM)/$(HB_COMPILER)
LIBS     := -L$(LIBDIR) \
            -lhbastlex \
            -lhbpp \
            -lhbrtl \
            -lhbvm \
            -lhbcommon \
            -lharbour \
            -lm

TARGETS := smoke snapshot rename builder-test

.PHONY: all clean build tests suite

all: build

smoke: smoke.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIBS) $(LDFLAGS) -o $@

snapshot: test_snapshot.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIBS) $(LDFLAGS) -o $@

rename: test_rename.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIBS) $(LDFLAGS) -o $@

builder-test: test_builder.c
	$(CC) $(CFLAGS) $(INCLUDES) -I/usr/local/include $< $(LIBS) -L/usr/local/lib -lcmocka $(LDFLAGS) -o $@

clean:
	rm -f $(TARGETS)

build: $(TARGETS)

tests: build
	(cd $(ROOT) && ./tests/ast/smoke)
	(cd $(ROOT) && ./tests/ast/rename)
	(cd $(ROOT) && ./tests/ast/snapshot)
	(cd $(ROOT) && ./tests/ast/builder-test)
