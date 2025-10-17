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

CMOCKA_INC  ?= -I/usr/local/include
CMOCKA_LIBS ?= -L/usr/local/lib -lcmocka

TARGETS := smoke snapshot rename builder-test pp-trace

.PHONY: all clean build tests suite

all: build

smoke: ast_smoke_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

snapshot: ast_snapshot_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

rename: ast_rename_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

builder-test: ast_builder_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

pp-trace: ast_preprocessor_trace_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

clean:
	rm -f $(TARGETS)

build: $(TARGETS)

tests: build
	(cd $(ROOT) && ./tests/ast/smoke)
	(cd $(ROOT) && ./tests/ast/rename)
	(cd $(ROOT) && ./tests/ast/snapshot)
	(cd $(ROOT) && ./tests/ast/builder-test)
	(cd $(ROOT) && ./tests/ast/pp-trace)
