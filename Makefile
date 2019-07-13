# Disable built-in rules and variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables
.SUFFIXES:

.PHONY: clean all
.DEFAULT_GOAL := all

# Compiler

CONFIG := debug
THREADS_CONFIG := pthread

CC := clang
ifeq (, $(shell which clang 2> /dev/null))
  CC := gcc
endif

TRACE = @echo '> $@';

# Macros

BUILD_DIR := build
DEP_DIR := $(BUILD_DIR)/$(CONFIG)/dep
OBJ_DIR := $(BUILD_DIR)/$(CONFIG)/obj
OUT_DIR := $(BUILD_DIR)/$(CONFIG)/bin

#SRC := $(shell find src/ -name '*.c')
SRC := src/demo.c
OBJ := ${SRC:.c=.o}
DEP := ${SRC:.c=.d}
OUT := demo

CFLAGS := -std=gnu89 -Wall -Werror
LDFLAGS :=
INCLUDES :=

MAKE_DIRS := \
$(DEP_DIR)/$(shell dirname $(DEP)) \
$(OBJ_DIR)/$(shell dirname $(OBJ)) \
$(OUT_DIR)/$(shell dirname $(OUT))

AUTODEP = -MD -MP -MF $(DEP_DIR)/$*.d

# Behaviour tweaks

CFLAGS += -DCO_MULTI_SRC_Q_N=4

# Configs

ifeq (debug,$(CONFIG))
  CFLAGS += -ggdb
  CFLAGS += -DCO_DBG_ASSERTIONS=1
  # Enable for extra debug prints: 
  # CFLAGS += -DCO_DBG_TRACES=1
endif

ifeq (release,$(CONFIG))
  CFLAGS += -O3
endif

ifeq (pthread, $(THREADS_CONFIG))
  CFLAGS += -DCO_THREADS_PTHREAD
  LDFLAGS += -pthread
endif

# Targets

$(MAKE_DIRS):
	$(TRACE)mkdir -p $@

$(OUT_DIR)/$(OUT): $(OBJ_DIR)/$(OBJ)
	$(TRACE)$(CC) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.c Makefile | $(MAKE_DIRS)
	$(TRACE)$(CC) $(AUTODEP) $(INCLUDES) $(CFLAGS) -c $< -o $@

all: $(OUT_DIR)/$(OUT)

doc:
	$(TRACE)doxygen

clean:
	$(TRACE)rm -rf build doc

-include $(DEP_DIR)/$(DEP)
