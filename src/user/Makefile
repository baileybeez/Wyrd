# - User Program(s) build file

USER_SRC	   := $(SRC_DIR)/user
KERNEL_SRC_DIR := $(SRC_DIR)/kernel
COMMON_SRC_DIR := $(SRC_DIR)/common
USER_BUILD     := $(BUILD_DIR)/user
OBJ_DIR        := $(USER_BUILD)/obj

SRCS	:= sample.c 
OBJS	:= $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

LINKER_SCRIPT := $(USER_SRC)/linker.ld

FEATURES := 
CFLAGS   := $(TARGET_CFLAGS) $(FEATURES) -I$(KERNEL_SRC_DIR) -I$(COMMON_SRC_DIR) -MMD -MP
CFLAGS   += -g3 -gdwarf-4 -O0 -fno-omit-frame-pointer

LDFLAGS  := -T $(LINKER_SCRIPT) -nostdlib
LIBS 	 := $(shell $(TARGET_CC) -print-libgcc-file-name)

.PHONY: all apps clean
all: apps

apps: $(USER_BUILD)/sample

$(USER_BUILD)/sample: $(OBJS) $(LINKER_SCRIPT)
	@mkdir -p $(dir $@)
	$(TARGET_LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJ_DIR)/%.o: $(USER_SRC)/%.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(USER_BUILD)

-include $(DEPS)
