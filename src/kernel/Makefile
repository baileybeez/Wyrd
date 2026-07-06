# - Kernel build 
#		compiles src/kernel/*.c and entry.s, links via linker.ld
# 		into build/kernel/kernel.elf.

KERNEL_SRC_DIR := $(SRC_DIR)/kernel
COMMON_SRC_DIR := $(SRC_DIR)/common
KERNEL_BUILD   := $(BUILD_DIR)/kernel
OBJ_DIR        := $(KERNEL_BUILD)/obj

# explicit source lists
C_SRCS   := main.c vga.c
ASM_SRCS := entry.asm

C_OBJS   := $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst %.asm,$(OBJ_DIR)/%.o,$(ASM_SRCS))
OBJS     := $(ASM_OBJS) $(C_OBJS)
DEPS     := $(C_OBJS:.o=.d)

LINKER_SCRIPT := $(KERNEL_SRC_DIR)/linker.ld

CFLAGS  := $(TARGET_CFLAGS) -I$(KERNEL_SRC_DIR) -I$(COMMON_SRC_DIR) -MMD -MP
LDFLAGS := -T $(LINKER_SCRIPT) -nostdlib
LIBS    := -lgcc

KERNEL_ELF := $(KERNEL_BUILD)/kernel.elf

.PHONY: all clean
all: $(KERNEL_ELF)

$(KERNEL_ELF): $(OBJS) $(LINKER_SCRIPT)
	@mkdir -p $(dir $@)
	$(TARGET_LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJ_DIR)/%.o: $(KERNEL_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(KERNEL_SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $< -o $@

clean:
	rm -rf $(KERNEL_BUILD)

-include $(DEPS)
