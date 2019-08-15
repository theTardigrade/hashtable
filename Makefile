SRC_DIR = src/
SRC_FILES = $(addsuffix .c, $(addprefix $(SRC_DIR), \
	table \
))
BUILD_DIR = lib/
BUILD_OPTS = -Wall -lm
BUILD_FILE = $(BUILD_DIR)table.o
DEBUG_OPTS=--leak-check=full --show-reachable=yes --track-origins=yes -v

.PHONY: build

default: clean build

build:
	mkdir -p $(BUILD_DIR)
	gcc -o $(BUILD_FILE) -c $(SRC_FILES) $(BUILD_OPTS)

clean:
	rm -f $(BUILD_FILE)