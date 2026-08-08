# - User Program(s) build file

# these need defined per-app
APPNAME  	:= strtests
APP_SRC_DIR	:= $(SRC_DIR)/user/strtests
SRCS		:= main.c 
# 

COMMON_SRC_DIR := $(SRC_DIR)/common
APP_BUILD      := $(BUILD_DIR)/user/$(APPNAME)
APP		 	   := $(APP_BUILD)/$(APPNAME)
OBJ_DIR        := $(APP_BUILD)/obj

OBJS	:= $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)
CRT0	:= $(BUILD_DIR)/libs/crt0.o
LIBWYRD := $(BUILD_DIR)/libs/libwyrd.a

LINKER_SCRIPT := $(SRC_DIR)/libs/libwyrd/user.ld

INCLUDES := -I$(COMMON_SRC_DIR) -I$(SRC_DIR)/libs/libwyrd/include
FEATURES := 
CFLAGS   := $(USER_CFLAGS) $(FEATURES) $(INCLUDES)

LDFLAGS  := -T $(LINKER_SCRIPT) -nostdlib -Wl,--gc-sections
LIBS 	 := $(LIBWYRD) $(shell $(TARGET_CC) -print-libgcc-file-name)

.PHONY: all clean
all: $(APP)

$(APP): $(OBJS) $(LINKER_SCRIPT)
	@mkdir -p $(dir $@)
	$(TARGET_LD) $(LDFLAGS) -o $@ $(CRT0) $(OBJS) $(LIBS)
	@cp $@ $(ROOT_FS)/$(APPNAME)

$(OBJ_DIR)/%.o: $(APP_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(TARGET_CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(USER_BUILD)

-include $(DEPS)
