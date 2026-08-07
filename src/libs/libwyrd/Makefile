# libwyrd Makefile
#

LIB_SRC		:= $(SRC_DIR)/libs/libwyrd/src
LIB_BUILD	:= $(BUILD_DIR)/libs/libwyrd
OBJ_DIR		:= $(LIB_BUILD)/objs
LIBWYRD		:= $(BUILD_DIR)/libs/libwyrd.a
CRT0		:= $(BUILD_DIR)/libs/crt0.o
CRT0_SRC	:= $(SRC_DIR)/libs/libwyrd/crt

SRCS		:= $(LIB_SRC)/io.c \
			   $(LIB_SRC)/mem.c \
			   $(LIB_SRC)/syscall.c

INC 		:= -I$(SRC_DIR)/libs/libwyrd/include -I$(SRC_DIR)/common

OBJS		:= $(patsubst $(LIB_SRC)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS		:= $(OBJS:.o=.d)

.PHONY: all
all: $(LIBWYRD) $(CRT0)

$(LIBWYRD): $(OBJS)
	@mkdir -p $(dir $@)
	$(TARGET_AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(LIB_SRC)/%.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(USER_CFLAGS) $(INC) -c $< -o $@

$(CRT0): $(CRT0_SRC)/crt0.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $< -o $@

-include $(DEPS)
