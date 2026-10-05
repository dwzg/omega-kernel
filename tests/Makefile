# Host tests for the Omega kernel. No devkitARM needed: everything here is
# built with the PC's C compiler, with AddressSanitizer and UBSan enabled.
#
#   make            build and run the unit tests and the UI simulator tests
#   make unit       unit tests only
#   make sim        UI simulator tests only (needs mtools to build the SD image)
#   make screenshots  write PNG screenshots of every screen to build/screens
#   make clean
#
# Variables: CC (default cc), SANITIZE=0 to build without sanitizers.

ROOT     := $(abspath ..)
BUILD    := build
CC       ?= cc
SANITIZE ?= 1
PYTHON   ?= python3

# A fixed version keeps the golden screenshots stable across releases.
CFLAGS := -std=gnu17 -g -O1 -Wall -Wextra -Werror -Wshadow -Wstrict-prototypes \
          -Wmissing-prototypes -Wno-unused-parameter -fno-common \
          -I. -Ihost -I$(ROOT)/src -I$(ROOT)/third_party/fatfs \
          -DREPO_ROOT='"$(ROOT)"' \
          -DKERNEL_VERSION='"2.0.0"' -DKERNEL_REVISION='"sim"'
ifeq ($(SANITIZE),1)
CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined
LDFLAGS += -fsanitize=address,undefined
endif

# Kernel sources that build on the host unchanged.
CORE_SRCS  := $(wildcard $(ROOT)/src/core/*.c)
PATCH_SRCS := $(ROOT)/src/patch/patch.c $(ROOT)/src/patch/rom_fixes.c \
              $(ROOT)/src/data/irq_patch_db.c $(ROOT)/src/data/save_type_db.c
GFX_SRCS   := $(wildcard $(ROOT)/src/gfx/*.c)

UNIT_SRCS := $(wildcard unit/*.c) host/host.c $(CORE_SRCS) $(PATCH_SRCS) $(GFX_SRCS)


.PHONY: all unit sim screenshots clean
all: unit sim

unit: $(BUILD)/unit_tests
	@$(BUILD)/unit_tests

$(BUILD)/unit_tests: $(UNIT_SRCS) $(wildcard *.h host/*.h unit/*.h) | $(BUILD)
	$(CC) $(CFLAGS) $(UNIT_SRCS) $(LDFLAGS) -o $@

$(BUILD):
	@mkdir -p $@

-include sim/sim.mk

clean:
	rm -rf $(BUILD)
