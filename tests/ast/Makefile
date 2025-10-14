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

TARGET := smoke
SOURCE := smoke.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIBS) $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)
