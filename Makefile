GNUARM_PREFIX ?= /Users/tuanem/.local/arm-none-eabi/bin/arm-none-eabi-

CC = $(GNUARM_PREFIX)gcc
CFLAGS = -Wall -W -pedantic -Wno-long-long -Os -fcall-used-r9 -fpie
CFLAGS += -Iinclude -Iinclude/apple \
          -Idrivers/aes -Idrivers/nand_llb -Idrivers/nand -Idrivers/nand_fw \
          -Idrivers/nor0 -Idrivers/dump -Idrivers/blob -Idrivers/fb -Idrivers/version -Idrivers/device -I.

DEBUG ?= 1
ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG_LOG=1
else
CFLAGS += -DDEBUG_LOG=0
endif

BUILDDIR = build

LD = $(GNUARM_PREFIX)gcc
LDFLAGS = -L$(BUILDDIR) -nostdlib -Tscript.ld -Wl,--no-warn-rwx-segments
LDLIBS = -lp

AR = $(GNUARM_PREFIX)ar
ARFLAGS = crus

OBJCOPY = $(GNUARM_PREFIX)objcopy

SOURCES = \
	link.c \
	main.c \
	drivers/aes/aes.c \
	drivers/nand_llb/nand_llb.c \
	drivers/nand/nand.c \
	drivers/nand_fw/nand_fw.c \
	drivers/nor0/nor0.c \
	drivers/blob/blob.c \
	drivers/dump/dump.c \
	drivers/fb/fb.c \
	drivers/version/version.c \
	drivers/device/device.c

LIBSOURCES = \
	lib/atoi.c \
	lib/div.c \
	lib/hex.c \
	lib/memcmp.c \
	lib/memmem.c \
	lib/memmove.c \
	lib/memset.c \
	lib/strcmp.c \
	lib/strlen.c \
	lib/xtoi.c

OBJECTS = $(addprefix $(BUILDDIR)/, $(SOURCES:.c=.o) cache.o)
LIBOBJECTS = $(addprefix $(BUILDDIR)/, $(LIBSOURCES:.c=.o))

TARGET_ELF = $(BUILDDIR)/Lynx32.elf
TARGET_BIN = $(BUILDDIR)/Lynx32.bin
LIBP = $(BUILDDIR)/libp.a
ENTRY_O = $(BUILDDIR)/entry.o

.PHONY: all release debug clean distclean

all: $(TARGET_BIN)

release:
	$(MAKE) clean
	$(MAKE) DEBUG=0

debug:
	$(MAKE) clean
	$(MAKE) DEBUG=1

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBP): $(LIBOBJECTS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(TARGET_ELF): $(OBJECTS) | $(ENTRY_O) $(LIBP)
	@mkdir -p $(BUILDDIR)
	$(LD) -o $@ $(LDFLAGS) $(OBJECTS) $(LDLIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "%%%%% done building"

clean:
	-rm -rf $(BUILDDIR) *.o drivers/*/*.o lib/*.o *.elf *.a payload

distclean: clean
