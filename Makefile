# Default path
BOOT_PATH ?= BOOT/BOOT.ELF

PAYLOAD = PROTPWN
OUT_DIR = BIEXEC-SYSTEM
EE_BIN = payload.elf
EE_BIN_RAW = payload.bin

EE_INCS = -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I$(PS2SDK)/sbv/include -Iinclude -I$(PS2SDK)/ports/include
EE_LIBS =
EE_OBJS = main.o

EE_LINKFILE = linkfile
ELF_FILES += loader.elf
KPATCH_FILES += kpatch_0100.img kpatch_0101.img

# C compiler flags
EE_CFLAGS := -D_EE -O2 -G0 -Wall $(EE_CFLAGS)
EE_CFLAGS = -D_EE -Os -G0 -Wall -Werror -fdata-sections -ffunction-sections $(EE_INCS)
EE_LDFLAGS += -Wl,-zmax-page-size=128 -Wl,--gc-sections -s

ifdef BOOT_PATH
 EE_CFLAGS += -DBOOT_PATH=\"$(BOOT_PATH)\"
endif

# Reduce binary size by using newlib-nano
EE_NEWLIB_NANO = 1
NEWLIB_NANO = 1

EE_OBJS_DIR = obj/
EE_ASM_DIR = asm/
EE_SRC_DIR = src/

EE_OBJS += $(IRX_FILES:.irx=_irx.o)
EE_OBJS += $(ELF_FILES:.elf=_elf.o)
EE_OBJS += $(KPATCH_FILES:.img=_img.o)
EE_OBJS := $(EE_OBJS:%=$(EE_OBJS_DIR)%)

.PHONY: all clean

all: $(PAYLOAD)

clean:
	$(MAKE) -C loader clean
	rm -rf $(EE_OBJS_DIR) $(EE_BIN) $(EE_BIN_RAW) $(OUT_DIR)

BIN2C = $(PS2SDK)/bin/bin2c

$(PAYLOAD): clean $(EE_BIN_RAW)
	mkdir $(OUT_DIR)
# Fakepack the payload
	utils/fakepack.py $(EE_BIN_RAW) $(OUT_DIR)/$@
# Generate OSBROWS manifest
	printf "101\n$(PAYLOAD)\n007a0000\n" > $(OUT_DIR)/OSBROWS

# ELF loader
loader.elf:
	$(MAKE) -C loader/$<

%loader_elf.c: loader.elf
	$(BIN2C) $(*:$(EE_SRC_DIR)%=loader/%)loader.elf $@ $(*:$(EE_SRC_DIR)%=%)loader_elf

# KPatch files
%_img.c:
	$(BIN2C) $(*:$(EE_SRC_DIR)%=kpatch/%).img $@ $(*:$(EE_SRC_DIR)%=%)_img

$(EE_ASM_DIR):
	@mkdir -p $@

$(EE_OBJS_DIR):
	@mkdir -p $@

$(EE_OBJS_DIR)%.o: $(EE_SRC_DIR)%.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)%.o : $(EE_SRC_DIR)%.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_BIN_RAW): $(EE_BIN)
	$(EE_OBJCOPY) -O binary -v $< $@

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
