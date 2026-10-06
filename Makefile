# GreaterBash - build file
# Recipes use ">" instead of TAB (see .RECIPEPREFIX), so copy-paste is safe.
.RECIPEPREFIX = >

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2
INC     := -Imain/inc -Imath_parser/inc -Iutils/inc
LDLIBS  := -lm

TARGET  := greaterbash
BUILD   := build

# main/src/text.c is a standalone experiment with its own main(), not part of the shell
SRC := $(filter-out main/src/text.c,$(wildcard main/src/*.c)) \
       $(wildcard math_parser/src/*.c) \
       $(wildcard utils/src/*.c)
OBJ := $(SRC:%.c=$(BUILD)/%.o)
DEP := $(OBJ:.o=.d)

.PHONY: all debug run clean

all: $(TARGET)

$(TARGET): $(OBJ)
> $(CC) $(OBJ) -o $@ $(LDLIBS)

$(BUILD)/%.o: %.c
> @mkdir -p $(dir $@)
> $(CC) $(CFLAGS) $(INC) -MMD -MP -c $< -o $@

debug: CFLAGS := -Wall -Wextra -g -O0 -fsanitize=address,undefined
debug: LDLIBS += -fsanitize=address,undefined
debug: clean $(TARGET)

run: $(TARGET)
> ./$(TARGET)

clean:
> rm -rf $(BUILD) $(TARGET)

-include $(DEP)
