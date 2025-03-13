QUESTION=3.12a
TOOLS_DIR=../../tools/$(QUESTION)
TEST_DIR=../../test/$(QUESTION)
BUILD_DIR=$(TOOLS_DIR)/build
SRC_CC=$(TOOLS_DIR)/$(QUESTION).cc
EXEC=$(BUILD_DIR)/main.out

CC=g++
CFLAGS=-Wall -Wextra -g -std=c++17

all: build

build: $(EXEC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(EXEC): $(SRC_CC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC_CC) -o $(EXEC)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

test: $(EXEC)
	@echo "Running tests..."
	@for file in $(wildcard $(TEST_DIR)/*.txt); do \
		echo "Testing with $$file"; \
		$(EXEC) $$file; \
	done
