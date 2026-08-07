# -

export BUILD_DIR := $(abspath build)
export SRC_DIR   := $(abspath src)
export TEST_DIR  := $(abspath tests)
export ROOT_FS	 := $(abspath root)

JOBS ?= $(shell nproc)

export ASM := nasm

export TARGET := i686-elf

export TARGET_CFLAGS := -std=c99 -g -ffreestanding -nostdlib -nostdinc \
						-Wall -Wextra -Wno-unused-parameter 
export TARGET_LFLAGS :=
export TARGET_LIBS   :=

export USER_ARCH	 := -march=i686 -mtune=generic -m32
export USER_NOFP	 := -mno-80387 -mno-mmx -mno-sse -mno-sse2
export USER_CFLAGS	 := $(TARGET_CFLAGS) $(USER_ARCH) $(USER_NOFP) \
						-fno-pic -fno-pie -fno-common -fno-stack-protector \
						-fno-asynchronous-unwind-tables \
						-fno-tree-loop-distribute-patterns \
						-ffunction-sections -fdata-sections \
						-MMD -MP

export TARGET_CC      := $(TARGET)-gcc
export TARGET_CXX 	  := $(TARGET)-g++
export TARGET_LD  	  := $(TARGET)-gcc
export TARGET_LINKER  := $(TARGET)-ld
export TARGET_AR	  := $(TARGET)-ar
export TARGET_OBJDUMP := $(TARGET)-objdump
export TARGET_OBJCOPY := $(TARGET)-objcopy

BINUTILS_VER := binutils-2.42
BINUTILS_ZIP := $(BINUTILS_VER).tar.gz
BINUTILS_URL := https://ftp.gnu.org/gnu/binutils/$(BINUTILS_ZIP)

GCC_VER := gcc-13.3.0
GCC_ZIP := $(GCC_VER).tar.gz
GCC_URL := https://ftp.gnu.org/gnu/gcc/$(GCC_VER)/$(GCC_ZIP)

GDB_VER 	:= gdb-14.2
GDB_ZIP 	:= $(GDB_VER).tar.gz
GDB_URL     := https://ftp.gnu.org/gnu/gdb/$(GDB_ZIP)
