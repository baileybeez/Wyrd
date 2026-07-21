# assemble a custom-bootable disk image from the kernel and bootloader
#
# Layout on disk:
#   Sector 0        (LBA 0)      : stage1 (exactly 512 bytes, MBR)
#   Sectors 1..N    (LBA 1+)     : stage2
#   Everything else              : zeros (room for kernel, filesystem later)
# 

CUSTOM_SRC		:= $(CURDIR)/
CUSTOM_BUILD	:= $(BUILD_DIR)/boot

STAGE1_BIN		:= $(CUSTOM_BUILD)/stage1.bin
STAGE2_BIN		:= $(CUSTOM_BUILD)/stage2.bin

DISK_IMAGE		:= $(BUILD_DIR)/bee.img
DISK_SIZE_MB	:= 10

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

$(STAGE2_BIN): $(CUSTOM_SRC)/stage2.asm
	@mkdir -p $(dir $@)
	$(ASM) -f bin $< -o $@

clean:
	rm -rf $(CUSTOM_BUILD) $(DISK_IMAGE)
