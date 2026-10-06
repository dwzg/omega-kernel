# UI simulator (included by tests/Makefile).

SIM_SRCS := sim/scenarios.c sim/platform_sim.c sim/disk_sim.c sim/stubs.c host/host.c \
            $(CORE_SRCS) $(GFX_SRCS) $(wildcard $(ROOT)/src/ui/*.c) $(ROOT)/src/data/save_type_db.c \
            $(addprefix $(ROOT)/src/loader/,directory.c library_files.c game_info.c sd_paths.c \
                save_choice.c save_backup.c buffers.c boot_messages.c)
FATFS_OBJS := $(addprefix $(BUILD)/fatfs/,ff.o ffunicode.o ffsystem.o)

SIM_IMAGE  := $(BUILD)/sd.img
SIM_GOLDEN := sim/golden.txt

# FatFs is third-party code: build it without our extra warnings.
$(BUILD)/fatfs/%.o: $(ROOT)/third_party/fatfs/%.c $(wildcard $(ROOT)/third_party/fatfs/*.h)
	@mkdir -p $(@D)
	$(CC) $(filter-out -Werror -W%,$(CFLAGS)) -c $< -o $@

$(BUILD)/sim: $(SIM_SRCS) $(FATFS_OBJS) $(wildcard sim/*.h host/*.h $(ROOT)/src/*/*.h)
	$(CC) $(CFLAGS) -Isim $(SIM_SRCS) $(FATFS_OBJS) $(LDFLAGS) -o $@

$(SIM_IMAGE): sim/make_sd_image.py | $(BUILD)
	@$(PYTHON) sim/make_sd_image.py $@

SIM_DEPS := $(BUILD)/sim $(SIM_IMAGE)

sim: $(SIM_DEPS)
	@rm -rf $(BUILD)/screens-failed
	@$(BUILD)/sim $(SIM_IMAGE) $(SIM_GOLDEN)

# Accept the current screens as the new reference (look at them first:
# make screenshots).
update-golden: $(SIM_DEPS)
	@$(BUILD)/sim $(SIM_IMAGE) $(SIM_GOLDEN) --update

# PNG screenshots of every screen, e.g. for the documentation.
screenshots: $(SIM_DEPS)
	@rm -rf $(BUILD)/screens
	@$(BUILD)/sim $(SIM_IMAGE) $(SIM_GOLDEN) --screens $(BUILD)/screens || true
	@$(PYTHON) $(ROOT)/tools/ppm2png.py $(BUILD)/screens/*.ppm
	@echo screenshots in $(BUILD)/screens

.PHONY: update-golden
