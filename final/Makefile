# 默认隐藏命令输出
MAKEFLAGS += -s

RM       = rm -rf
BUILD_DIR = $(CURDIR)/build
TEST_DIR = $(CURDIR)/test
FMJ_DIR = $(TEST_DIR)/fmj
AST_DIR = $(TEST_DIR)/ast
SEMANT_DIR = $(TEST_DIR)/semant
IR_DIR = $(TEST_DIR)/ir
QUAD_DIR = $(TEST_DIR)/quad
SSA_DIR = $(TEST_DIR)/ssa
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
	$(RM) $(AST_DIR)/*
	$(RM) $(SEMANT_DIR)/*
	$(RM) $(IR_DIR)/*
	$(RM) $(QUAD_DIR)/*
	$(RM) $(SSA_DIR)/*

veryclean: clean 
	$(RM) $(BUILD_DIR)

rebuild: veryclean build


# 运行相关规则
.PHONY: run run-one

run: $(MAIN_EXE)
	$(MAKE) fmj2ast
	$(MAKE) ast2semant
	$(MAKE) semant2ir
	$(MAKE) ir2quad
	$(MAKE) quad2ssa
	@printf "\n$(COLOR_GREEN)All tasks completed successfully!$(COLOR_RESET)\n"

run-one: $(MAIN_EXE)
	$(MAKE) fmj2ast-one
	$(MAKE) ast2semant-one
	$(MAKE) semant2ir-one
	$(MAKE) ir2quad-one
	$(MAKE) quad2ssa-one


# 编译相关规则
.PHONY: fmj2ast-one ast2semant-one semant2ir-one ir2quad-one quad2ssa-one fmj2ast ast2semant semant2ir ir2quad quad2ssa

# 修改为只传递文件名，不包含路径和扩展名
fmj2ast-one: $(MAIN_EXE)
	cd $(TEST_DIR) && \
	$(MAIN_EXE) "fmj2ast" "${FILE}" $(REDIRECT); \
	cd .. $(REDIRECT)

ast2semant-one: $(MAIN_EXE)
	cd $(TEST_DIR) && \
	$(MAIN_EXE) "ast2semant" "${FILE}" $(REDIRECT); \
	cd .. $(REDIRECT)

semant2ir-one: $(MAIN_EXE)
	cd $(TEST_DIR) && \
	$(MAIN_EXE) "semant2ir" "${FILE}" $(REDIRECT); \
	cd .. $(REDIRECT)

ir2quad-one: $(MAIN_EXE)
	cd $(TEST_DIR) && \
	$(MAIN_EXE) "ir2quad" "${FILE}" $(REDIRECT); \
	cd .. $(REDIRECT)

quad2ssa-one: $(MAIN_EXE)
	cd $(TEST_DIR) && \
	$(MAIN_EXE) "quad2ssa" "${FILE}" $(REDIRECT); \
	cd .. $(REDIRECT)

fmj2ast: $(MAIN_EXE)
	@cd $(TEST_DIR) && \
	total=$$(ls $(FMJ_DIR)/*.fmj | wc -l); \
	count=0; \
	for file in $$(ls $(FMJ_DIR)/*.fmj); do \
		count=$$((count + 1)); \
		printf "\033[2K$(COLOR_BLUE)Compiling FMJ to AST: [%-50s] %d/%d$(COLOR_RESET)\r" $$(printf '#%.0s' $$(seq 1 $$((count * 50 / total)))) $$count $$total; \
		filename=$$(basename $$file | cut -d. -f1); \
		$(MAKE) -C $(CURDIR) fmj2ast-one FILE=$$filename $(REDIRECT); \
	done; \
	printf "\033[2K$(COLOR_GREEN)FMJ to AST Compilation Completed Successfully!$(COLOR_RESET)\n"

ast2semant: $(MAIN_EXE)
	@cd $(TEST_DIR) && \
	total=$$(ls $(AST_DIR)/*.ast | wc -l); \
	count=0; \
	for file in $$(ls $(AST_DIR)/*.ast); do \
		count=$$((count + 1)); \
		printf "\033[2K$(COLOR_YELLOW)Compiling AST to Semant: [%-50s] %d/%d$(COLOR_RESET)\r" $$(printf '#%.0s' $$(seq 1 $$((count * 50 / total)))) $$count $$total; \
		filename=$$(basename $$file | cut -d. -f1); \
		$(MAKE) -C $(CURDIR) ast2semant-one FILE=$$filename $(REDIRECT); \
	done; \
	printf "\033[2K$(COLOR_GREEN)AST to Semant Compilation Completed Successfully!$(COLOR_RESET)\n"

semant2ir: $(MAIN_EXE)
	@cd $(TEST_DIR) && \
	total=$$(ls $(SEMANT_DIR)/*.ast | wc -l); \
	count=0; \
	for file in $$(ls $(SEMANT_DIR)/*.ast); do \
		count=$$((count + 1)); \
		printf "\033[2K$(COLOR_BLUE)Compiling AST to IR: [%-50s] %d/%d$(COLOR_RESET)\r" $$(printf '#%.0s' $$(seq 1 $$((count * 50 / total)))) $$count $$total; \
		filename=$$(basename $$file | cut -d. -f1); \
		$(MAKE) -C $(CURDIR) semant2ir-one FILE=$$filename $(REDIRECT); \
	done; \
	printf "\033[2K$(COLOR_GREEN)AST to IR Compilation Completed Successfully!$(COLOR_RESET)\n"

ir2quad: $(MAIN_EXE)
	@cd $(TEST_DIR) && \
	total=$$(ls $(IR_DIR)/*.irp | wc -l); \
	count=0; \
	for file in $$(ls $(IR_DIR)/*.irp); do \
		count=$$((count + 1)); \
		printf "\033[2K$(COLOR_YELLOW)Compiling IR to Quad: [%-50s] %d/%d$(COLOR_RESET)\r" $$(printf '#%.0s' $$(seq 1 $$((count * 50 / total)))) $$count $$total; \
		filename=$$(basename $$file | cut -d. -f1); \
		$(MAKE) -C $(CURDIR) ir2quad-one FILE=$$filename $(REDIRECT); \
	done; \
	printf "\033[2K$(COLOR_GREEN)IR to Quad Compilation Completed Successfully!$(COLOR_RESET)\n"

quad2ssa: $(MAIN_EXE)
	@cd $(TEST_DIR) && \
	total=$$(ls $(QUAD_DIR)/*.4-xml.quad | wc -l); \
	count=0; \
	for file in $$(ls $(QUAD_DIR)/*.4-xml.quad); do \
		count=$$((count + 1)); \
		printf "\033[2K$(COLOR_BLUE)Compiling Quad to SSA: [%-50s] %d/%d$(COLOR_RESET)\r" $$(printf '#%.0s' $$(seq 1 $$((count * 50 / total)))) $$count $$total; \
		filename=$$(basename $$file | cut -d. -f1); \
		$(MAKE) -C $(CURDIR) quad2ssa-one FILE=$$filename $(REDIRECT); \
	done; \
	printf "\033[2K$(COLOR_GREEN)Quad to SSA Compilation Completed Successfully!$(COLOR_RESET)\n"


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
