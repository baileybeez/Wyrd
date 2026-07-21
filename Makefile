include config.mk
include toolchain.mk

.PHONY: all help clean

all: help

help:
	@echo "Targets:"
	@echo "  make toolchain           Build binutils + GCC cross-compiler for $(TARGET)."
	@echo "  make verify-toolchain    Compile test/helloworld.c and confirm the"
	@echo ""
	@echo "Overrides:"
	@echo "  JOBS=N                   Parallelism for toolchain build (default: nproc)."

kernel: | $(GCC_INSTALLED)
	$(MAKE) -C $(SRC_DIR)/kernel

iso: kernel
	$(MAKE) -C boot/grub/ iso

img: kernel
	$(MAKE) -C boot/custom/ img

clean:
	rm -rf $(BUILD_DIR)/test $(BUILD_DIR)/kernel $(BUILD_DIR)/bee.iso 
