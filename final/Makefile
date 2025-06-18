# 默认隐藏命令输出
MAKEFLAGS += -s

RM       = rm -rf
BUILD_DIR = $(CURDIR)/build
TEST_DIR = $(CURDIR)/test
FMJ_DIR = $(TEST_DIR)/fmj
AST_DIR = $(TEST_DIR)/ast
SEMANT_DIR = $(TEST_DIR)/semant
MAIN_EXE = $(BUILD_DIR)/tools/main/main

# 可选的 VERBOSE 模式
VERBOSE ?= 0
REDIRECT = $(if $(filter 1,$(VERBOSE)),,> /dev/null 2>&1)

# 定义颜色
COLOR_RESET = \033[0m
COLOR_GREEN = \033[32m
COLOR_YELLOW = \033[33m
COLOR_BLUE = \033[34m


# 构建相关规则
.PHONY: build clean veryclean rebuild

build:
	cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja

clean:
	find $(AST_DIR) $(SEMANT_DIR) -maxdepth 1 -type f \( \
		-name "*.ast" -o -name "*.out"\
		\) -exec $(RM) {} \;

veryclean: clean 
	$(RM) $(BUILD_DIR)

rebuild: veryclean build


# 运行相关规则
.PHONY: run run-one

run: $(MAIN_EXE)
	$(MAKE) fmj2ast
	$(MAKE) ast2semant
	# $(MAKE) ast2ir
	@printf "\n$(COLOR_GREEN)All tasks completed successfully!$(COLOR_RESET)\n"

run-one: $(MAIN_EXE)
	$(MAKE) fmj2ast-one
	$(MAKE) ast2semant-one
	# $(MAKE) ast2ir-one


# 编译相关规则
.PHONY: fmj2ast-one ast2semant-one ast2ir-one fmj2ast ast2semant ast2ir

# 修改为只传递文件名，不包含路径和扩展名
fmj2ast-one: $(MAIN_EXE)
	cd $(TEST_DIR) && \
	$(MAIN_EXE) "fmj2ast" "${FILE}" $(REDIRECT); \
	cd .. $(REDIRECT)

ast2semant-one: $(MAIN_EXE)
	cd $(TEST_DIR) && \
	$(MAIN_EXE) "ast2semant" "${FILE}" $(REDIRECT); \
	cd .. $(REDIRECT)

ast2ir-one: $(MAIN_EXE)
	cd $(TEST_DIR) && \
	$(MAIN_EXE) "ast2ir" "${FILE}" $(REDIRECT); \
	cd .. $(REDIRECT)

fmj2ast: $(MAIN_EXE)
	@cd $(TEST_DIR) && \
	total=$$(ls fmj/*.fmj | wc -l); \
	count=0; \
	for file in $$(ls fmj/*.fmj); do \
		count=$$((count + 1)); \
		printf "\033[2K$(COLOR_BLUE)Compiling FMJ to AST: [%-50s] %d/%d$(COLOR_RESET)\r" $$(printf '#%.0s' $$(seq 1 $$((count * 50 / total)))) $$count $$total; \
		filename=$$(basename $$file | cut -d. -f1); \
		$(MAKE) -C $(CURDIR) fmj2ast-one FILE=$$filename $(REDIRECT); \
	done; \
	printf "\033[2K$(COLOR_GREEN)FMJ to AST Compilation Completed Successfully!$(COLOR_RESET)\n"

ast2semant: $(MAIN_EXE)
	@cd $(TEST_DIR) && \
	total=$$(ls ast/*.ast | wc -l); \
	count=0; \
	for file in $$(ls ast/*.ast); do \
		count=$$((count + 1)); \
		printf "\033[2K$(COLOR_YELLOW)Compiling AST to Semant: [%-50s] %d/%d$(COLOR_RESET)\r" $$(printf '#%.0s' $$(seq 1 $$((count * 50 / total)))) $$count $$total; \
		filename=$$(basename $$file | cut -d. -f1); \
		$(MAKE) -C $(CURDIR) ast2semant-one FILE=$$filename $(REDIRECT); \
	done; \
	printf "\033[2K$(COLOR_GREEN)AST to Semant Compilation Completed Successfully!$(COLOR_RESET)\n"

# ast2ir: $(MAIN_EXE)
# 	@cd $(TEST_DIR) && \
# 	total=$$(ls semant/*.ast | wc -l); \
# 	count=0; \
# 	for file in $$(ls semant/*.ast); do \
# 		count=$$((count + 1)); \
# 		printf "\033[2K$(COLOR_BLUE)Compiling AST to IR: [%-50s] %d/%d$(COLOR_RESET)\r" $$(printf '#%.0s' $$(seq 1 $$((count * 50 / total)))) $$count $$total; \
# 		filename=$$(basename $$file | cut -d. -f1); \
# 		$(MAKE) -C $(CURDIR) ast2ir-one FILE=$$filename $(REDIRECT); \
# 	done; \
# 	printf "\033[2K$(COLOR_GREEN)AST to IR Compilation Completed Successfully!$(COLOR_RESET)\n"


# 提交相关规则
.PHONY: handin

handin:
	if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-final.zip" \
		docs/report.pdf include lib
