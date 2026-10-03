CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip

CFLAGS ?=-Wall -Werror -std=gnu11 -D_GNU_SOURCE -g -DUSE_DEV_LIB -DUSE_SPI -I $(CURDIR)/Unity/src -I $(CURDIR)/lg -I $(CURDIR)/oled -I $(CURDIR)/oled/Config -I $(CURDIR)/oled/Fonts -I $(CURDIR)/oled/GUI
LDFLAGS ?=-L$(CURDIR)/lg -lm -llgpio


# add in the raspberry pi gpio linux headers.
# This is needed for the lg library to compile correctly.
TARGET_HEADERS ?= $(CURDIR)/lg_headers
ifneq ($(TARGET_HEADERS),)
export CPATH := $(TARGET_HEADERS)$(if $(CPATH),:$(CPATH))
endif

PROG=mesh_statusd
TEST=test_main

BUILD_DIR = $(CURDIR)/build
SRC_DIRS = $(CURDIR) $(CURDIR)/oled $(CURDIR)/oled/Config $(CURDIR)/oled/Fonts $(CURDIR)/oled/GUI
ALL_SRC = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
UNITY_SRC = $(CURDIR)/Unity/src/unity.c
TEST_SRC = $(wildcard test_*.c)
SRCS := $(filter-out test_%.c, $(ALL_SRC))
OBJ_FILES := $(addprefix $(BUILD_DIR)/, $(notdir $(SRCS:.c=.o)))
OBJS := $(filter-out $(BUILD_DIR)/test_%.o, $(OBJ_FILES))

DUPES := $(shell printf '%s\n' $(notdir $(SRCS)) | sort | uniq -d)
ifneq ($(DUPES),)
$(error Duplicate source filenames across SRC_DIRS, cannot flatten build dir: $(DUPES))
endif

vpath %.c $(SRC_DIRS)

.PHONY: all lg clean test

all: lg $(PROG)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

lg:
	$(MAKE) -C $(CURDIR)/lg/ CC=$(CC) AR=$(AR) STRIP=$(STRIP) CROSS_COMPILE=$(CROSS_COMPILE)

clean:
	-rm -rf $(BUILD_DIR)
	-rm $(PROG) $(TEST)
	-$(MAKE) -C $(CURDIR)/lg/ clean

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(PROG)
	$(CC) $(CFLAGS) $(TEST_SRC) $(UNITY_SRC) -o $(TEST)
	$(CURDIR)/test_main
