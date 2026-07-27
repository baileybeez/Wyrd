# - Kernel build 
#		compiles src/kernel/*.c and entry.s, links via linker.ld
# 		into build/kernel/kernel.elf.

KERNEL_SRC_DIR := $(SRC_DIR)/kernel
COMMON_SRC_DIR := $(SRC_DIR)/common
KERNEL_BUILD   := $(BUILD_DIR)/kernel
OBJ_DIR        := $(KERNEL_BUILD)/obj

# explicit source lists
ASM_SRCS := entry.asm \
			arch/i686/descriptor_tables.asm \
			arch/i686/enterUserMode.asm \
			arch/i686/isr_stubs.asm \
			arch/i686/irq_stubs.asm \
			scheduler/switchContext.asm 
C_SRCS   := main.c \
			syscall.c \
			drivers/serial/serial.c \
			drivers/video/vga.c \
			drivers/input/keyboard.c \
			drivers/input/keyboardMap.c \
			lib/kprintf.c \
			lib/logger.c \
			lib/mem.c \
			arch/i686/arch.c \
			arch/i686/gdt.c \
			arch/i686/idt.c \
			arch/i686/irq.c \
			arch/i686/isr.c \
			arch/i686/pic.c \
			arch/i686/pit.c \
			arch/i686/paging.c \
			arch/i686/syscallEntry.c \
			arch/i686/ticks.c \
			arch/i686/tss.c \
			boot/multiboot.c \
			boot/bootInfo.c \
			mm/pmm.c \
			mm/heap.c \
			fs/fat16/fat16.c \
			scheduler/thread.c \
			scheduler/scheduler.c \
			userProg.c 

C_OBJS   := $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst %.asm,$(OBJ_DIR)/%.o,$(ASM_SRCS))
OBJS     := $(ASM_OBJS) $(C_OBJS)
DEPS     := $(C_OBJS:.o=.d)
TEST_OBJ := /workspace/build/test/userProg.o

LINKER_SCRIPT := $(KERNEL_SRC_DIR)/linker.ld

FEATURES := -DkFAT16_DirListing 
CFLAGS   := $(TARGET_CFLAGS) $(FEATURES) -I$(KERNEL_SRC_DIR) -I$(COMMON_SRC_DIR) -MMD -MP
CFLAGS   += -g3 -gdwarf-4 -O0 -fno-omit-frame-pointer

LDFLAGS  := -T $(LINKER_SCRIPT) -nostdlib
LIBS     := -lgcc

KERNEL_ELF := $(KERNEL_BUILD)/kernel.elf
KERNEL_BIN := $(KERNEL_BUILD)/kernel.bin

.PHONY: all clean
all: $(KERNEL_ELF) $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(TARGET_OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(OBJS) $(LINKER_SCRIPT) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(TARGET_LD) $(LDFLAGS) -o $@ $(OBJS) $(TEST_OBJ) $(LIBS)

$(OBJ_DIR)/%.o: $(KERNEL_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(KERNEL_SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $< -o $@

clean:
	rm -rf $(KERNEL_BUILD)

-include $(DEPS)
