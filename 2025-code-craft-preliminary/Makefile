QUIET ?= @
ROOT_DIR = .
CODE_PATH = $(ROOT_DIR)/cpp
EXECUTABLE = code_craft
CMAKE_PATH = $(CODE_PATH)/cmake_build
DATA_PATH = $(ROOT_DIR)/data
INTERACTOR_PATH = $(ROOT_DIR)/interactor

.PHONY: build run clean submit

build: clean
	$(QUIET) cmake -S $(CODE_PATH) -B $(CMAKE_PATH)
	$(QUIET) make -C $(CMAKE_PATH)

RUN_FLAG :=
ifdef D
	RUN_FLAG += -d $(D)
endif
ifdef R
	RUN_FLAG += -r $(R)
endif

INPUT_FILE :=
ifdef INPUT
	INPUT_FILE += $(DATA_PATH)/$(INPUT)
else
	INPUT_FILE += $(DATA_PATH)/sample.in
endif

# usage: make run DEBUG="1 1" REPLAY="1 2" INPUT=sample_practice.in
run: build
	$(QUIET) echo "\033[1;34m--- Running ---\033[0m"
	python3 run.py $(INTERACTOR_PATH)/linux/interactor $(INPUT_FILE) $(CODE_PATH)/$(EXECUTABLE) $(RUN_FLAG)

clean:
	rm -rf $(CMAKE_PATH) $(CODE_PATH)/${EXECUTABLE}
#	rm -rf $(ROOT_DIR)/replay/*.txt
#	rm -rf $(ROOT_DIR)/debug.txt
#	rm -rf $(ROOT_DIR)/result.txt

SUBMIT_PREFIX :=
ifdef P
	SUBMIT_PREFIX += $(P)
else
	SUBMIT_PREFIX += "_submission"
endif

# usage: make submit P="TZXSUBMIT"
submit:
	(cd $(CODE_PATH) && zip -r ../$(SUBMIT_PREFIX)_$(shell date +%m%d_%H%M%S).zip * -x "cmake*" -x "$(EXECUTABLE)")
