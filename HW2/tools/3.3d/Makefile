QUESTION=3.3d
TOOLS_DIR=../../tools/$(QUESTION)
TEST_DIR=../../test/$(QUESTION)
BUILD_DIR=$(TOOLS_DIR)/build
SRC_LEX=$(TOOLS_DIR)/$(QUESTION).lex
SRC_YACC=$(TOOLS_DIR)/$(QUESTION).yacc
LEX_OUT=$(BUILD_DIR)/lexer.c
YACC_OUT=$(BUILD_DIR)/parser.tab.c
EXEC=$(BUILD_DIR)/$(QUESTION)

LEX=flex
YACC=bison
CC=g++
CFLAGS=-Wall -Wextra -g

all: build

build: $(EXEC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LEX_OUT): $(SRC_LEX) | $(BUILD_DIR)
	$(LEX) -o $(LEX_OUT) $(SRC_LEX)

$(YACC_OUT): $(SRC_YACC) | $(BUILD_DIR)
	$(YACC) -d $(SRC_YACC) -o $(YACC_OUT)

$(EXEC): $(LEX_OUT) $(YACC_OUT)
	$(CC) $(CFLAGS) $(YACC_OUT) $(LEX_OUT) -o $(EXEC) -lfl

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

test: $(EXEC)
	@for file in $(wildcard $(TEST_DIR)/*.txt); do \
		$(EXEC) < $$file; \
	done