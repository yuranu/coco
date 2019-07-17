# Disable built-in rules and variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables
.SUFFIXES:

.PHONY: clean all mkdir
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
DOT := ${SRC:.c=.dot/callgraph.dot}
OUT := demo

CFLAGS := -std=gnu89 -Wall -Werror
LDFLAGS :=
INCLUDES :=

AUTODEP = -MD -MP -MF $(DEP_DIR)/$*.d

# Behaviour tweaks

CFLAGS += -DCO_MULTI_SRC_Q_N=4

# Configs

ifeq (debug,$(CONFIG))
  CFLAGS += -ggdb
  CFLAGS += -DCO_DBG_ASSERTIONS=1
  # Enable for extra debug prints: 
  CFLAGS += -DCO_DBG_VERBOSE=0
endif

ifeq (release,$(CONFIG))
  CFLAGS += -O3
endif

ifeq (pthread, $(THREADS_CONFIG))
  CFLAGS += -DCO_THREADS_PTHREAD
  LDFLAGS += -pthread
endif

# Macros
COMPILE = $(CC) $(AUTODEP) $(INCLUDES) $(CFLAGS)
LINK    = $(CC) $(LDFLAGS)

define builddir
  $(shell echo $($1) | xargs -d ' ' -I{} echo $($1_DIR)/{})
endef

define multi_dirname
	$(shell echo $($1) | xargs -d ' ' -I{} dirname {})
endef

# Targets

-include $(DEP_DIR)/$(DEP)

mkdir:
	$(TRACE)mkdir -p $(call multi_dirname,MAKE_DIRS)

# Binary generation
$(OUT_DIR)/$(OUT): MAKE_DIRS = $(call builddir,OBJ) $(call builddir,DEP) $(call builddir,OUT)
$(OUT_DIR)/$(OUT): $(call builddir,OBJ)
	$(TRACE)$(LINK) $< -o $@
$(OBJ_DIR)/%.o: %.c Makefile | mkdir
	$(TRACE)$(COMPILE) -c $< -o $@

all: $(OUT_DIR)/$(OUT)

doc:
	$(TRACE)doxygen

clean:
	$(TRACE)rm -rf build doc
