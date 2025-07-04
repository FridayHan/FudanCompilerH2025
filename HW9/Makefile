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
	$(RM) test/*5.* \
	$(RM) $(CURDIR)/test/tmp/

rebuild: clean build

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-hw1.zip" \
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
	@mkdir -p $(CURDIR)/test/tmp
	@cp $(CURDIR)/test/input_example/*.quad $(CURDIR)/test/tmp/
	@cp $(CURDIR)/test/input_example/k9/*.clr $(CURDIR)/test/tmp/
	@cd $(CURDIR)/test/tmp && \
	for file in $$(ls .); do \
		if [ "$${file#*.}" = "4-prepared-xml.quad" ]; then \
			echo "Reading $${file} and $${file%%.*}.4-xml.clr"; \
			$(MAIN) "$${file%%.*}"; \
		fi; \
	done

run-assem: $(MAIN)
	@cd $(CURDIR)/test/tmp && \
	for file in $$(ls .); do \
		if [ "$${file#*.}" = "4-prepared-xml.quad" ]; then \
			echo "Reading $${file} and $${file%%.*}.4-xml.clr"; \
			arm-linux-gnueabihf-gcc -mcpu=cortex-a72 -Wall -Wextra --static \
				-o "$${file%%.*}" "$${file%%.*}.s" ../../vendor/libsysy/libsysy32.s -lm; \
			echo "Running the final assembly program........."; \
			qemu-arm -B 0x1000 "$${file%%.*}" || true; \
		fi; \
	done

FILE=test1

run-one: $(MAIN)
	@mkdir -p $(CURDIR)/test/tmp
	@cp $(CURDIR)/test/input_example/$(FILE).4-prepared-xml.quad $(CURDIR)/test/tmp/
	@cp $(CURDIR)/test/input_example/k9/$(FILE).4-xml.clr $(CURDIR)/test/tmp/
	@cd $(CURDIR)/test/tmp && \
	echo "Reading $(FILE).4-prepared-xml.quad and $(FILE).4-xml.clr"; \
	$(MAIN) "$(FILE)"

run-one-assem: $(MAIN)
	@cd $(CURDIR)/test/tmp && \
	echo "Compiling assembly for $(FILE)..."; \
	arm-linux-gnueabihf-gcc -mcpu=cortex-a72 -Wall -Wextra --static \
		-o "$(FILE)" "$(FILE).s" ../../vendor/libsysy/libsysy32.s -lm; \
	echo "Running the final assembly program........."; \
	qemu-arm -B 0x1000 "$(FILE)" || true
