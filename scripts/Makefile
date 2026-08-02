NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
MAKEFLAGS += -j$(NPROC)

ROOT_DIR := $(abspath ..)
BUILD_DIR := $(ROOT_DIR)/objs/scripts
GEN_MODEL_DIR := $(CURDIR)/gen_model
GEN_MODEL_OBJDIR := $(BUILD_DIR)/gen_model
COLLISION_DIR := $(GEN_MODEL_DIR)/collision
COLLISION_OBJDIR := $(GEN_MODEL_OBJDIR)/collision

CC_BASE := g++
ifneq ($(shell command -v ccache 2>/dev/null),)
CC := ccache $(CC_BASE)
else
CC := $(CC_BASE)
endif

AR := ar -rcs
RM := rm -rf
CFLAGS := -std=c++20 -Wall -Wextra -Werror -MMD -MP
GEN_MODEL_IFLAGS := -I$(ROOT_DIR)/scripts -I$(ROOT_DIR)/includes/raylib
COLLISION_IFLAGS := -I$(ROOT_DIR)/scripts
LFLAGS := -L$(ROOT_DIR)/includes/raylib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

GEN_MODEL_CORE_SRCS := \
	$(GEN_MODEL_DIR)/gen_types.cpp \
	$(GEN_MODEL_DIR)/asteroid_generator.cpp \
	$(GEN_MODEL_DIR)/asteroid_small.cpp \
	$(GEN_MODEL_DIR)/asteroid_big.cpp \
	$(GEN_MODEL_DIR)/asset_writer.cpp
GEN_MODEL_CORE_OBJS := $(patsubst $(GEN_MODEL_DIR)/%.cpp,$(GEN_MODEL_OBJDIR)/%.o,$(GEN_MODEL_CORE_SRCS))
GEN_MODEL_MAIN_OBJ := $(GEN_MODEL_OBJDIR)/main.o
GEN_MODEL_LIB := $(BUILD_DIR)/libgen_model.a
GEN_MODEL_BIN := $(GEN_MODEL_OBJDIR)/gen_model

COLLISION_CORE_SRCS := \
	$(COLLISION_DIR)/model_importer.cpp \
	$(COLLISION_DIR)/collision_writer.cpp
COLLISION_CORE_OBJS := $(patsubst $(COLLISION_DIR)/%.cpp,$(COLLISION_OBJDIR)/%.o,$(COLLISION_CORE_SRCS))
COLLISION_MAIN_OBJ := $(COLLISION_OBJDIR)/main.o
COLLISION_LIB := $(BUILD_DIR)/libgen_collision.a
COLLISION_BIN := $(COLLISION_OBJDIR)/gen_collision

.PHONY: all gen_model gen_model_lib gen_collision gen_collision_lib clean

all: gen_model

gen_model: $(GEN_MODEL_BIN)
	@cd "$(ROOT_DIR)" && "$(GEN_MODEL_BIN)"

gen_model_lib: $(GEN_MODEL_LIB)

gen_collision: $(COLLISION_BIN)
	@if [ -z "$(strip $(INPUT))" ]; then \
		echo "Usage: make -C scripts gen_collision INPUT=input1.obj,input2.obj"; \
		exit 2; \
	fi
	@cd "$(ROOT_DIR)" && "$(COLLISION_BIN)" "$(INPUT)"

gen_collision_lib: $(COLLISION_LIB)

$(GEN_MODEL_LIB): $(GEN_MODEL_CORE_OBJS) | $(BUILD_DIR)
	$(AR) $@ $^

$(COLLISION_LIB): $(COLLISION_CORE_OBJS) | $(BUILD_DIR)
	$(AR) $@ $^

$(GEN_MODEL_BIN): $(GEN_MODEL_MAIN_OBJ) $(GEN_MODEL_LIB) | $(GEN_MODEL_OBJDIR)
	$(CC) $(CFLAGS) $(GEN_MODEL_MAIN_OBJ) $(GEN_MODEL_LIB) $(LFLAGS) -o $@

$(COLLISION_BIN): $(COLLISION_MAIN_OBJ) $(COLLISION_LIB) | $(COLLISION_OBJDIR)
	$(CC) $(CFLAGS) $(COLLISION_MAIN_OBJ) $(COLLISION_LIB) -o $@

$(GEN_MODEL_OBJDIR)/%.o: $(GEN_MODEL_DIR)/%.cpp | $(GEN_MODEL_OBJDIR)
	$(CC) $(CFLAGS) $(GEN_MODEL_IFLAGS) -c $< -o $@

$(COLLISION_OBJDIR)/%.o: $(COLLISION_DIR)/%.cpp | $(COLLISION_OBJDIR)
	$(CC) $(CFLAGS) $(COLLISION_IFLAGS) -c $< -o $@

$(BUILD_DIR) $(GEN_MODEL_OBJDIR) $(COLLISION_OBJDIR):
	mkdir -p $@

clean:
	@$(RM) $(BUILD_DIR)

-include $(GEN_MODEL_CORE_OBJS:.o=.d) $(GEN_MODEL_MAIN_OBJ:.o=.d) \
	$(COLLISION_CORE_OBJS:.o=.d) $(COLLISION_MAIN_OBJ:.o=.d)
