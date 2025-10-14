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
            -lhbnortl \
            -lhbvm \
            -lhbcommon \
            -lharbour \
            -lm

TARGETS := smoke snapshot

.PHONY: all clean

all: $(TARGETS)

smoke: smoke.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIBS) $(LDFLAGS) -o $@

snapshot: test_snapshot.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIBS) $(LDFLAGS) -o $@

clean:
	rm -f $(TARGETS)
