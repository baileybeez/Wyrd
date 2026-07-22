# assemble a custom-bootable disk image from the kernel and bootloader
#
# Layout on disk:
#   Sector 0        (LBA 0)      : stage1 (exactly 512 bytes, MBR)
#   Sectors 1..N    (LBA 1+)     : stage2
#   Everything else              : zeros (room for kernel, filesystem later)
# 

CUSTOM_SRC		:= $(CURDIR)
KERNEL_SRC 		:= $(CURDIR)/../../src/kernel/
CUSTOM_BUILD	:= $(BUILD_DIR)/boot
INCLUDE_COMMON  := $(CURDIR)/../../src/common/

STAGE1_BIN		:= $(CUSTOM_BUILD)/stage1.bin
STAGE2_BIN		:= $(CUSTOM_BUILD)/stage2.bin

DISK_IMAGE		:= $(BUILD_DIR)/bee.img
DISK_SIZE_MB	:= 10

STAGE2_MAX		:= 8192

STAGE2_ASM     	:= stage2.asm
STAGE2_C       	:= main.c
SHARED_C		:= $(KERNEL_SRC)/drivers/serial/serial.c \
				   $(KERNEL_SRC)/lib/kprintf.c 

STAGE2_OBJDIR	:= $(CUSTOM_BUILD)/objs

S2_C_OBJS		:= $(patsubst %.c,$(STAGE2_OBJDIR)/%.o,$(STAGE2_C))
S2_ASM_OBJS 	:= $(patsubst %.asm,$(STAGE2_OBJDIR)/%.o,$(STAGE2_ASM))
SHARED_OBJS		:= $(patsubst $(KERNEL_SRC)/%.c,$(STAGE2_OBJDIR)/shared/%.o,$(SHARED_C))
STAGE2_OBJS		:= $(S2_ASM_OBJS) $(S2_C_OBJS) $(SHARED_OBJS)

CFLAGS			:= -m32 -ffreestanding -nostdlib -fno-pic -fno-stack-protector \
                   -fno-builtin -Wall -Wextra -O2 -I $(INCLUDE_COMMON) -I $(KERNEL_SRC)

LINKER_SCRIPT	:= $(CUSTOM_SRC)/linker.ld

.PHONY: img clean

img: $(DISK_IMAGE)

$(DISK_IMAGE): $(STAGE1_BIN) $(STAGE2_BIN)
	dd if=/dev/zero of=$@ bs=1M count=$(DISK_SIZE_MB) status=none
	dd if=$(STAGE1_BIN) of=$@ conv=notrunc status=none
	dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none

$(STAGE1_BIN): $(CUSTOM_SRC)/stage1.asm
	@mkdir -p $(dir $@)
	$(ASM) -f bin $< -o $@
	@size=$$(stat -c%s $@); \
	if [ $$size -ne 512 ]; then \
	   echo "ERROR: stage1.bin is $$size bytes, must be exactly 512"; \
	   exit 1; \
	fi

$(STAGE2_BIN): $(STAGE2_OBJS) $(LINKER_SCRIPT)
	@mkdir -p $(dir $@)
	$(TARGET_LINKER) -T $(LINKER_SCRIPT) -o $@ $(STAGE2_OBJS)
	@size=$$(stat -c%s $@); \
	if [ $$size -gt $(STAGE2_MAX) ]; then \
		echo "ERROR: stage2.bin is $$size bytes, exceeds $(STAGE2_MAX) budget"; \
		echo "       bump kStage2Sectors in stage1.asm and STAGE2_MAX here." \
		exit 1; \
	fi

$(STAGE2_OBJDIR)/%.o: $(CUSTOM_SRC)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $< -o $@

$(STAGE2_OBJDIR)/%.o: $(CUSTOM_SRC)/%.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(CFLAGS) -c $< -o $@

$(STAGE2_OBJDIR)/shared/%.o: $(KERNEL_SRC)/%.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(CUSTOM_BUILD) $(DISK_IMAGE)
