RM       = rm -rf
MAKEFLAGS = --no-print-directory

.PHONY: build clean rebuild handin

BUILD_DIR = $(CURDIR)/build

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja
# cd $(BUILD_DIR) && ninja -v

clean: 
	@$(RM) build ; \
	$(RM) test/*3*; \
	$(RM) test/*4.quad; \
	$(RM) test/*4-xml.quad; \
	$(RM) test/*4-ssa.quad; \
	$(RM) test/*4-ssa2.quad; \
	$(RM) test/*4-ssa-back.quad; \
	$(RM) test/*4-block.quad; \
	$(RM) test/*4-prepared.quad; \
	$(RM) test/*4-alloced.quad; \
	$(RM) test/*4-rpi.quad; \
	$(RM) test/*4-rpi-xml.quad; \
	$(RM) test/*4-xml.clr;

rebuild: clean build

deepclean: clean 
	${RM} test/*.4.quad

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-hw8.zip" \
	  docs/report.pdf include lib

.PHONY: run run-one gentests gentest-one patchdemo 

GENAST = $(CURDIR)/../HW5/build/tools/generatetests/gentests
GENIR = $(CURDIR)/../HW5/build/tools/main/main
GENQUAD = $(CURDIR)/../HW6/build/tools/main/main
GENSSA = $(CURDIR)/../HW7/build/tools/main/main
MAIN = $(BUILD_DIR)/tools/main/main
GENTESTS = $(BUILD_DIR)/tools/generatetests/gentests
PATCHDEMO = $(BUILD_DIR)/tools/patchdemo/patchdemo

run: $(MAIN)
	cd $(CURDIR)/test && \
	for file in input_example/*.4-ssa-xml.quad; do \
		filename=$$(basename $$file .4-ssa-xml.quad); \
		cp $$file ./$$filename.4-ssa-xml.quad; \
		echo "Reading $$filename.4-ssa-xml.quad"; \
		$(MAIN) "$$filename"; \
		rm -f ./$$filename.4-ssa-xml.quad; \
	done

run-5: $(MAIN)
	cd $(CURDIR)/test && \
	for file in input_example/*.4-ssa-xml.quad; do \
		filename=$$(basename $$file .4-ssa-xml.quad); \
		cp $$file ./$$filename.4-ssa-xml.quad; \
		echo "Reading $$filename.4-ssa-xml.quad"; \
		$(MAIN) -k 5 "$$filename"; \
		rm -f ./$$filename.4-ssa-xml.quad; \
	done

FILE=test1

run-one-5: $(MAIN)
	cd $(CURDIR)/test && \
	echo "Reading ${FILE}.4-ssa-xml.quad"; \
	$(MAIN) -k 5 "${FILE}"; \
	cd .. > /dev/null 2>&1 

run-one: $(MAIN)
	cd $(CURDIR)/test && \
	echo "Reading ${FILE}.4-ssa-xml.quad"; \
	$(MAIN) "${FILE}"; \
	cd .. > /dev/null 2>&1 

