ROOT        := ../..
HB_PLATFORM ?= linux
HB_COMPILER ?= gcc

CC      ?= gcc
CFLAGS  ?=
LDFLAGS ?=

INCLUDES := -I$(ROOT)/include -I$(ROOT)/src/compiler
LIBDIR   := $(ROOT)/lib/$(HB_PLATFORM)/$(HB_COMPILER)
LIBS     := -L$(LIBDIR) \
            -lhbcplr \
            -lhbpp \
            -lhbrtl \
            -lhbvm \
            -lhbcommon \
            -lharbour \
            -lm

CMOCKA_INC  ?= -I/usr/local/include
CMOCKA_LIBS ?= -L/usr/local/lib -lcmocka

TARGETS := pp-trace trace-events hbmk2-fixtures compilebuf-tests hbmk-ast-tests

.PHONY: all clean build tests suite

all: build

pp-trace: ast_preprocessor_trace_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

trace-events: ast_trace_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

hbmk2-fixtures: ast_hbmk2_fixtures_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

compilebuf-tests: ast_compilebuf_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

hbmk-ast-tests: ast_hbmk_ast_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CMOCKA_INC) $< $(LIBS) $(CMOCKA_LIBS) $(LDFLAGS) -o $@

clean:
	rm -f $(TARGETS)

build: $(TARGETS)

tests: build
	(cd $(ROOT) && ./tests/ast/pp-trace)
	(cd $(ROOT) && ./tests/ast/trace-events)
	(cd $(ROOT) && ./tests/ast/hbmk2-fixtures)
	(cd $(ROOT) && ./tests/ast/compilebuf-tests)
	(cd $(ROOT) && ./tests/ast/hbmk-ast-tests)
