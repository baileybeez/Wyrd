# -

export BUILD_DIR := $(abspath build)
export SRC_DIR   := $(abspath src)
export TEST_DIR  := $(abspath test)

JOBS ?= $(shell nproc)

export ASM := nasm

export TARGET := i686-elf

export TARGET_CFLAGS := -std=c99 -g -ffreestanding -Wall -Wextra
export TARGET_LFLAGS :=
export TARGET_LIBS   :=

export TARGET_CC      := $(TARGET)-gcc
export TARGET_CXX 	  := $(TARGET)-g++
export TARGET_LD  	  := $(TARGET)-gcc
export TARGET_LINKER  := $(TARGET)-ld
export TARGET_OBJDUMP := $(TARGET)-objdump
export TARGET_OBJCOPY := $(TARGET)-objcopy

BINUTILS_VER := binutils-2.42
BINUTILS_ZIP := $(BINUTILS_VER).tar.gz
BINUTILS_URL := https://ftp.gnu.org/gnu/binutils/$(BINUTILS_ZIP)

GCC_VER := gcc-13.2.0
GCC_ZIP := $(GCC_VER).tar.gz
GCC_URL := https://ftp.gnu.org/gnu/gcc/$(GCC_VER)/$(GCC_ZIP)

GDB_VER 	:= gdb-14.2
GDB_ZIP 	:= $(GDB_VER).tar.xz
GDB_URL     := https://ftp.gnu.org/gnu/gdb/$(GDB_ZIP)
