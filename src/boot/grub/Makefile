# assemble a GRUB-bootable ISO from the kernel and grub.cfg
#
# Staging layout under $(ISO_DIR):
#   boot/kernel.elf
#   boot/grub/grub.cfg
# 

GRUB_CFG_SRC := $(CURDIR)/grub.cfg

GRUB_BUILD   := $(BUILD_DIR)/grub
ISO_DIR      := $(GRUB_BUILD)/iso
ISO_OUT      := $(BUILD_DIR)/wyrd.iso

KERNEL_ELF   := $(BUILD_DIR)/kernel/kernel.elf

.PHONY: iso clean

iso: $(ISO_OUT)

$(ISO_OUT): $(KERNEL_ELF) $(GRUB_CFG_SRC)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF)   $(ISO_DIR)/boot/kernel.elf
	cp $(GRUB_CFG_SRC) $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO_DIR)

clean:
	rm -rf $(GRUB_BUILD) $(ISO_OUT)
