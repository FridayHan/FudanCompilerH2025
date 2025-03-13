# 变量定义
QUESTION=2.4b
TOOLS_DIR=../../tools/$(QUESTION)
TEST_DIR=../../test/$(QUESTION)
BUILD_DIR=$(TOOLS_DIR)/build
SRC=$(TOOLS_DIR)/$(QUESTION).lex
LEX_OUT=$(BUILD_DIR)/lex.yy.c
EXEC=$(BUILD_DIR)/$(QUESTION)

# 编译工具
LEX=flex
CC=gcc
CFLAGS=-Wall -Wextra -g

# 目标规则
all: build

build: $(EXEC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LEX_OUT): $(SRC) | $(BUILD_DIR)
	$(LEX) -o $(LEX_OUT) $(SRC)

$(EXEC): $(LEX_OUT)
	$(CC) $(CFLAGS) $(LEX_OUT) -o $(EXEC) -lfl

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

test: $(EXEC)
	@for file in $(wildcard $(TEST_DIR)/*.txt); do \
		$(EXEC) < $$file; \
	done

