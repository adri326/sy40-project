BUILD_DIR := build
SRC_DIR := src
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(SRC_FILES:$(SRC_DIR)/%.c=%.o)
EXE_NAME := sy40_project

.PHONY: default_target all clean

default_target: all

ifeq ($(OS), Windows_NT)
	EXE_NAME := $(EXE_NAME).exe
clean:
	del $(BUILD_DIR)\*.o $(BUILD_DIR)\$(EXE_NAME)
else
clean:
	if [ -d $(BUILD_DIR) ]; then rm $(BUILD_DIR)/*.o $(BUILD_DIR)/$(EXE_NAME); rmdir $(BUILD_DIR); fi
endif

all: $(BUILD_DIR)/$(EXE_NAME)

$(BUILD_DIR)/:
	mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(SRC_DIR)/*.h) | $(BUILD_DIR)/
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(EXE_NAME): $(BUILD_DIR)/$(OBJ_FILES) $(wildcard $(SRC_DIR)/*.h) | $(BUILD_DIR)/
	$(CC) $(CFLAGS) $< -o $@
