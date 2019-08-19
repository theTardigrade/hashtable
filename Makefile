SRC_DIR = src/
SRC_CODE_FILES = $(addsuffix .c, $(addprefix $(SRC_DIR), \
	table \
))
SRC_HEADER_FILES = $(addsuffix .h, $(addprefix $(SRC_DIR), \
	table \
))
BUILD_DIR = lib/
BUILD_OPTS = -Wall
BUILD_FILE = $(BUILD_DIR)table.o

.PHONY: build

default: clean build

build:
	mkdir -p $(BUILD_DIR)
	gcc -o $(BUILD_FILE) -c $(SRC_CODE_FILES) $(BUILD_OPTS)
	cp $(SRC_HEADER_FILES) $(BUILD_DIR)

clean:
	rm -f $(BUILD_FILE)