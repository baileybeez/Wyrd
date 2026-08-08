# - Kernel build 
#		compiles src/kernel/*.c and entry.s, links via linker.ld
# 		into build/kernel/kernel.elf.

KERNEL_SRC_DIR := $(SRC_DIR)/kernel
COMMON_SRC_DIR := $(SRC_DIR)/common
KERNEL_BUILD   := $(BUILD_DIR)/kernel
OBJ_DIR        := $(KERNEL_BUILD)/obj
FLAG_CACHE	   := $(KERNEL_BUILD)/.cflags

# explicit source lists
ASM_SRCS := entry.asm \
			arch/i686/descriptor_tables.asm \
			arch/i686/enterUserMode.asm \
			arch/i686/isr_stubs.asm \
			arch/i686/irq_stubs.asm \
			scheduler/switchContext.asm 
C_SRCS   := main.c \
			drivers/ata/ata.c \
			drivers/input/keyboard.c \
			drivers/input/keyboardMap.c \
			drivers/video/vga.c \
			drivers/serial/serial.c \
			loader/elf.c \
			loader/exec.c \
			lib/kprintf.c \
			lib/logger.c \
			lib/mem.c \
			lib/panic.c \
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
			mm/usercopy.c \
			fs/fat16/fat16.c \
			scheduler/thread.c \
			scheduler/scheduler.c \
			loader/elfSelfTest.c \
			loader/lifecycleTests.c \
			syscall/syscall.c \
			shell/shell.c

C_OBJS   := $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst %.asm,$(OBJ_DIR)/%.o,$(ASM_SRCS))
OBJS     := $(ASM_OBJS) $(C_OBJS)
DEPS     := $(C_OBJS:.o=.d)
DEPS 	 := $(OBJS:.o=.d)
LINKER_SCRIPT := $(KERNEL_SRC_DIR)/linker.ld

# FEATURES options: `-D<flag> ...`
# 	kIncludeSelfTests 	:: include all diagnostic / debug tests (i.e. ATA, FAT16, ELFLOAD)
# 	kSerialTrace		:: includes support for debug console via serial (QEMU)
FEATURES := -DkSerialTrace
CFLAGS   := $(TARGET_CFLAGS) $(FEATURES) -I$(KERNEL_SRC_DIR) -I$(COMMON_SRC_DIR) -MMD -MP
CFLAGS   += -g3 -gdwarf-4 -O0 -fno-omit-frame-pointer -fno-pic -fno-pie

LDFLAGS  := -T $(LINKER_SCRIPT) -nostdlib
LIBS     := -lgcc

KERNEL_ELF := $(KERNEL_BUILD)/kernel.elf
KERNEL_BIN := $(KERNEL_BUILD)/kernel.bin

.PHONY: all clean force
all: $(FLAG_CACHE) $(KERNEL_ELF) $(KERNEL_BIN)

$(FLAG_CACHE): force
	@mkdir -p $(dir $@)
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(TARGET_OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(FLAG_CACHE) $(OBJS) $(LINKER_SCRIPT) 
	@mkdir -p $(dir $@)
	$(TARGET_LD) $(LDFLAGS) -o $@ $(OBJS)  $(LIBS)

$(OBJ_DIR)/%.o: $(KERNEL_SRC_DIR)/%.c $(FLAG_CACHE)
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(KERNEL_SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $< -o $@

clean:
	rm -rf $(KERNEL_BUILD)

-include $(DEPS)
