# -----------------------------------------------------------------------------
# toolchain.mk: build binutils + GCC as a cross-compiler targeting $(TARGET).
#
# Layout:
#   toolchain/tarballs/         downloaded source archives (wget -c resumable)
#   toolchain/binutils-2.42/    extracted binutils source tree
#   toolchain/gcc-13.2.0/       extracted gcc source tree (+ downloaded prereqs)
#   toolchain/i686-elf/         installation prefix; final binaries live here
#   build/toolchain/binutils/   binutils build directory (out-of-tree)
#   build/toolchain/gcc/        gcc build directory (out-of-tree)
# -----------------------------------------------------------------------------

TOOLCHAIN_DIR       := toolchain
TOOLCHAIN_TARBALLS  := $(TOOLCHAIN_DIR)/tarballs
TOOLCHAIN_PREFIX    := $(abspath $(TOOLCHAIN_DIR)/$(TARGET))
TOOLCHAIN_BUILD_DIR := $(BUILD_DIR)/toolchain

# Put the cross-toolchain on PATH for this Make invocation and any sub-makes.
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)

# if these files exist, that phase is complete.
BINUTILS_INSTALLED := $(TOOLCHAIN_PREFIX)/bin/$(TARGET)-ld
GCC_INSTALLED      := $(TOOLCHAIN_PREFIX)/bin/$(TARGET)-gcc

.PHONY: toolchain toolchain_clean toolchain_binutils toolchain_gcc verify-toolchain

toolchain: toolchain_binutils toolchain_gcc

toolchain_binutils: $(BINUTILS_INSTALLED)

toolchain_gcc: $(GCC_INSTALLED)

# - download BinUtils and GCC (if needed)
$(TOOLCHAIN_TARBALLS)/$(BINUTILS_ZIP):
	mkdir -p $(TOOLCHAIN_TARBALLS)
	cd $(TOOLCHAIN_TARBALLS) && wget -c $(BINUTILS_URL)

$(TOOLCHAIN_TARBALLS)/$(GCC_ZIP):
	mkdir -p $(TOOLCHAIN_TARBALLS)
	cd $(TOOLCHAIN_TARBALLS) && wget -c $(GCC_URL)

# - extract files
$(TOOLCHAIN_DIR)/$(BINUTILS_VER)/.extracted: $(TOOLCHAIN_TARBALLS)/$(BINUTILS_ZIP)
	cd $(TOOLCHAIN_DIR) && tar -xf tarballs/$(BINUTILS_ZIP)
	touch $@

$(TOOLCHAIN_DIR)/$(GCC_VER)/.extracted: $(TOOLCHAIN_TARBALLS)/$(GCC_ZIP)
	cd $(TOOLCHAIN_DIR) && tar -xf tarballs/$(GCC_ZIP)
	touch $@

# - install pre-reqs
$(TOOLCHAIN_DIR)/$(GCC_VER)/.prereqs: $(TOOLCHAIN_DIR)/$(GCC_VER)/.extracted
	cd $(TOOLCHAIN_DIR)/$(GCC_VER) && ./contrib/download_prerequisites
	touch $@

# - build and install BinUtils
$(BINUTILS_INSTALLED): $(TOOLCHAIN_DIR)/$(BINUTILS_VER)/.extracted
	mkdir -p $(TOOLCHAIN_BUILD_DIR)/binutils
	cd $(TOOLCHAIN_BUILD_DIR)/binutils && \
	   $(abspath $(TOOLCHAIN_DIR)/$(BINUTILS_VER))/configure \
	      --prefix=$(TOOLCHAIN_PREFIX) \
	      --target=$(TARGET) \
	      --with-sysroot \
	      --disable-nls \
	      --disable-werror
	$(MAKE) -j$(JOBS) -C $(TOOLCHAIN_BUILD_DIR)/binutils
	$(MAKE) -C $(TOOLCHAIN_BUILD_DIR)/binutils install

# - build and install GCC
$(GCC_INSTALLED): $(TOOLCHAIN_DIR)/$(GCC_VER)/.prereqs $(BINUTILS_INSTALLED)
	mkdir -p $(TOOLCHAIN_BUILD_DIR)/gcc
	cd $(TOOLCHAIN_BUILD_DIR)/gcc && \
	   $(abspath $(TOOLCHAIN_DIR)/$(GCC_VER))/configure \
	      --prefix=$(TOOLCHAIN_PREFIX) \
	      --target=$(TARGET) \
	      --disable-nls \
	      --enable-languages=c,c++ \
	      --without-headers
	$(MAKE) -j$(JOBS) -C $(TOOLCHAIN_BUILD_DIR)/gcc all-gcc all-target-libgcc
	$(MAKE) -C $(TOOLCHAIN_BUILD_DIR)/gcc install-gcc install-target-libgcc

# - verify toolchain, builds a sample app and confirms bin format
VERIFY_OBJ := $(BUILD_DIR)/test/helloworld.o

verify-toolchain: $(GCC_INSTALLED)
	@mkdir -p $(BUILD_DIR)/test
	@echo "==> Compiling $(TEST_DIR)/helloworld.c with $(TARGET_CC)"
	$(TARGET_CC) -ffreestanding -c $(TEST_DIR)/helloworld.c -o $(VERIFY_OBJ)
	@echo ""
	@echo "==> $(TARGET_OBJDUMP) -f $(VERIFY_OBJ)"
	@$(TARGET_OBJDUMP) -f $(VERIFY_OBJ)
	@echo ""
	@if $(TARGET_OBJDUMP) -f $(VERIFY_OBJ) | grep -q 'file format elf32-i386'; then \
	   echo "OK: toolchain produces elf32-i386 objects."; \
	else \
	   echo "FAIL: unexpected object format. Toolchain build is not correct."; \
	   exit 1; \
	fi

# - clean only the build directories (fast), leaves downloads and extracted files
toolchain_clean:
	rm -rf $(TOOLCHAIN_BUILD_DIR)

# - clean entire toolchain
toolchain_distclean: toolchain_clean
	rm -rf $(TOOLCHAIN_DIR)/$(BINUTILS_VER) \
	       $(TOOLCHAIN_DIR)/$(GCC_VER) \
	       $(TOOLCHAIN_PREFIX)
