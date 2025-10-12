TARGET = qcs
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
PROFILE_DIR = profile
LOG_DIR = log
LOG_FILE = $(LOG_DIR)/$(TARGET).build_log

CC = gcc

CFLAGS =-std=c89 -pg -g -Wall -Wextra -pedantic -I$(INCLUDE_DIR)
LDFLAGS = -lm

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))


.PHONY: all clean newdir run bundle profile

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS) | newdir
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ LNK: $@ ]" | tee -a $(LOG_FILE)
	@$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | newdir
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ CC: $< ]" | tee -a $(LOG_FILE)
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ CFLAGS: $(CFLAGS) ] " | tee -a $(LOG_FILE)
	@$(CC) $(CFLAGS) -c $< -o $@

newdir:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(LOG_DIR)
	@touch $(LOG_FILE)

run: all
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ Running $(TARGET) ]" | tee -a $(LOG_FILE)
	@$(BUILD_DIR)/$(TARGET) $(ARGS)

profile: all
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ PROFILING: Creating profile dir ]" | tee -a $(LOG_FILE)
	@mkdir -p $(PROFILE_DIR)
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ PROFILING: Running profile.py to benchmark and create profiling reports ]" | tee -a $(LOG_FILE)
	@python scripts/profile.py
	@python scripts/plot_all_cpu.py
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ PROFILING: benchmarking and reports generated successfully ]" | tee -a $(LOG_FILE)

clean:
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ Cleaning up... ]" | tee -a $(LOG_FILE)
	@$(RM) -r $(BUILD_DIR)
	@$(RM) -r $(PROFILE_DIR)
	@$(RM) -r $(LOG_DIR)
	@$(RM) -f gmon.out callgraph.dot
	@$(RM) -f qcs.h test_bundle test_bundle.c

bundle:
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ BUNDLE: Running bundle.py to create qcs.h ]" | tee -a $(LOG_FILE)
	@python scripts/bundle.py
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ BUNDLE: qcs.h created successfully ]" | tee -a $(LOG_FILE)
