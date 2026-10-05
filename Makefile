# =============================================================================
# EZ-FLASH OMEGA kernel
#
#   make              build ezkernel.gba and ezkernel.bin (the upgrade file)
#   make test         build and run the host unit tests (needs a host C compiler)
#   make format       format all project sources with clang-format
#   make format-check fail if any source is not formatted
#   make lint         run cppcheck on the project sources
#   make docs         generate the API reference with Doxygen (build/docs)
#   make clean        remove all build output
#
# The cartridge build needs devkitPro with devkitARM and libgba; see
# docs/building.md. Variables you can override: V=1 (verbose), VERSION.
# =============================================================================

.SUFFIXES:
.DEFAULT_GOAL := all

TARGET    := ezkernel
BUILD     := build
VERSION   ?= $(shell cat VERSION 2>/dev/null || echo 0.0.0)
REVISION  ?= $(shell git rev-parse --short=7 HEAD 2>/dev/null || echo unknown)

# Project layout -------------------------------------------------------------
SOURCE_DIRS   := src src/hal src/core src/patch src/patch/payloads src/loader src/gfx \
                 src/ui src/platform/gba src/data third_party/fatfs
INCLUDE_DIRS  := src third_party/fatfs $(BUILD)/gen
BINARY_DIRS   := assets/firmware assets/patches assets/fonts

CFILES   := $(foreach d,$(SOURCE_DIRS),$(wildcard $(d)/*.c))
SFILES   := $(foreach d,$(SOURCE_DIRS),$(wildcard $(d)/*.s))
BINFILES := $(foreach d,$(BINARY_DIRS),$(wildcard $(d)/*.bin))

# Files we format and lint (third-party code is left as upstream wrote it).
OWN_SOURCES := $(shell find src tests tools -name '*.[ch]' 2>/dev/null | grep -v '/data/')

# Host-side targets (no devkitARM needed) -------------------------------------
CLANG_FORMAT ?= clang-format

.PHONY: test format format-check lint docs clean

test:
	@$(MAKE) --no-print-directory -C tests

format:
	@$(CLANG_FORMAT) -i $(OWN_SOURCES)

format-check:
	@$(CLANG_FORMAT) --dry-run --Werror $(OWN_SOURCES)

lint:
	@cppcheck --quiet --error-exitcode=1 --std=c11 --enable=warning,performance,portability \
		--inline-suppr --suppress=missingIncludeSystem -D__GBA__ -DIWRAM_CODE= -DEWRAM_BSS= \
		$(addprefix -I,src third_party/fatfs) src

docs:
	@mkdir -p $(BUILD)
	@doxygen Doxyfile

clean:
	@echo clean ...
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).gba $(TARGET).bin $(TARGET).map
	@$(MAKE) --no-print-directory -C tests clean

# Cartridge build -------------------------------------------------------------
ifneq ($(filter-out test format format-check lint docs clean,$(or $(MAKECMDGOALS),all)),)

ifeq ($(strip $(DEVKITARM)),)
$(error DEVKITARM is not set. Install devkitPro (gba-dev) or run ./build.sh; see docs/building.md)
endif
include $(DEVKITARM)/gba_rules

GAME_TITLE := EZKERNEL
GAME_CODE  := EZOK
MAKER_CODE := EZ

ARCH     := -mthumb -mthumb-interwork -mcpu=arm7tdmi -mtune=arm7tdmi
DEFINES  := -D__GBA__ -DKERNEL_VERSION=\"$(VERSION)\" -DKERNEL_REVISION=\"$(REVISION)\"
INCLUDES := $(addprefix -iquote ,$(INCLUDE_DIRS)) -isystem $(LIBGBA)/include
WARNINGS := -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wno-unused-parameter
CFLAGS   := -std=gnu17 -Os -g -fomit-frame-pointer $(ARCH) $(DEFINES) $(INCLUDES)
ASFLAGS  := -g $(ARCH)
LDFLAGS  := -g $(ARCH) -specs=gba.specs -Wl,-Map,$(BUILD)/$(TARGET).map

# FatFs is third-party code: build it with the plain -Wall set.
$(BUILD)/obj/third_party/%.o: WARNINGS := -Wall
# Generated asset/database sources may have long initialisers.
$(BUILD)/obj/src/data/%.o: WARNINGS := -Wall

GEN      := $(BUILD)/gen
BIN_OBJS := $(patsubst %.bin,$(GEN)/%_bin.o,$(notdir $(BINFILES)))
GEN_HDRS := $(BIN_OBJS:.o=.h)
OBJS     := $(CFILES:%.c=$(BUILD)/obj/%.o) $(SFILES:%.s=$(BUILD)/obj/%.o) $(BIN_OBJS)
DEPS     := $(OBJS:.o=.d)

vpath %.bin $(BINARY_DIRS)

ifeq ($(V),1)
Q :=
else
Q := @
endif

.PHONY: all size
all: $(TARGET).bin

$(TARGET).bin: $(TARGET).gba
	$(Q)cp $< $@
	@echo upgrade file ... $@

$(TARGET).gba: $(TARGET).elf

# Machine-code sanity checks on the linked kernel (see the scripts).
$(BUILD)/elf-checks.stamp: $(TARGET).elf tools/check_iwram_calls.py tools/check_thumb_entries.py
	@$(PREFIX)objdump -d -j .iwram $< | python3 tools/check_iwram_calls.py > /dev/null
	@$(PREFIX)readelf -s $< | python3 tools/check_thumb_entries.py
	@touch $@

$(TARGET).bin: $(BUILD)/elf-checks.stamp

$(TARGET).elf: $(OBJS) tools/memory_limits.ld
	@echo linking $@
	$(Q)$(CC) $(LDFLAGS) $(OBJS) -L$(LIBGBA)/lib -lgba tools/memory_limits.ld -o $@

$(BUILD)/obj/%.o: %.c | $(GEN_HDRS)
	@echo $<
	@mkdir -p $(@D)
	$(Q)$(CC) -MMD -MP -MF $(@:.o=.d) $(CFLAGS) $(WARNINGS) -c $< -o $@

$(BUILD)/obj/%.o: %.s
	@echo $<
	@mkdir -p $(@D)
	$(Q)$(CC) -MMD -MP -MF $(@:.o=.d) -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

# Binary assets -> assembly with bin2s -> object, plus a header declaring
#   <name>_bin, <name>_bin_end and <name>_bin_size.
$(GEN)/%_bin.s $(GEN)/%_bin.h: %.bin
	@echo $(notdir $<)
	@mkdir -p $(GEN)
	$(Q)bin2s -a 4 -H $(GEN)/$*_bin.h $< > $(GEN)/$*_bin.s

$(GEN)/%.o: $(GEN)/%.s
	$(Q)$(CC) -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

# Keep generated files (they are intermediate in make's eyes).
.SECONDARY:

size: $(TARGET).elf
	@$(PREFIX)size -A $< | grep -E '^\.(text|rodata|iwram|data|bss|sbss|ewram) '
	@end=$$($(PREFIX)nm $< | sed -n 's/^\([0-9a-f]*\) . __sbss_end__$$/\1/p'); \
		echo "EWRAM free: $$((0x02040000 - 0x$$end)) bytes"

-include $(DEPS)

endif
