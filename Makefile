include config.mk
include toolchain.mk

.PHONY: all help clean

all: help

help:
	@echo "Targets:"
	@echo "  make toolchain           	build binutils + GCC cross-compiler for $(TARGET)."
	@echo "                           		+ override: 'JOBS=N' (parallelism, default: nproc)"
	@echo "  make verify-toolchain    	compile tests/helloworld.c and confirm the toolchain"
	@echo ""
	@echo "  make kernel 			  	build only the kernel"
	@echo "  make grub-iso 		      	builds an ISO using the GRUB bootloader"
	@echo "  make wyrd-iso 		      	builds a disk image using the custom bootloader"
	@echo "  make apps					builds user apps for Wyrd"
	@echo ""

kernel: | $(GCC_INSTALLED)
	$(MAKE) -C $(SRC_DIR)/kernel

grub-iso: kernel
	$(MAKE) -C $(SRC_DIR)/boot/grub

wyrd: kernel rootfs
	$(MAKE) -C $(SRC_DIR)/boot/custom

rootfs:
	@mkdir -p $(ROOT_FS)/tmp
	@mkdir -p $(ROOT_FS)/bin
	@echo "hello from /tmp/file.txt via fat16" > $(ROOT_FS)/tmp/file.txt

libs: 
	$(MAKE) -C $(SRC_DIR)/libs/libwyrd

apps: rootfs
	$(MAKE) -C $(SRC_DIR)/user/sample

clean:
	rm -rf $(BUILD_DIR)
