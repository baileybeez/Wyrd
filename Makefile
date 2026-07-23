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

grub-iso: kernel
	$(MAKE) -C $(SRC_DIR)/boot/grub/ iso

bee-iso: kernel
	$(MAKE) -C $(SRC_DIR)/boot/custom/ iso

clean:
	rm -rf $(BUILD_DIR)
