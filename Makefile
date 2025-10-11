TARGET = qcs
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
LOG_FILE = $(BUILD_DIR)/$(TARGET).build_log

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
	@touch $(LOG_FILE)

run: all
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ Running $(TARGET) ]" | tee -a $(LOG_FILE)
	@$(BUILD_DIR)/$(TARGET) $(ARGS)

profile: all
	@echo "--- Starting Profiling and Resource Monitoring ---"

	@echo "\n[1/4] Running for time-series CPU/RAM monitoring..."
	@if command -v pidstat > /dev/null; then \
		echo "      -> Found pidstat. Logging usage every second."; \
		( $(BUILD_DIR)/$(TARGET) & PID=$$!; pidstat -p $$PID -r -u 1 > $(BUILD_DIR)/resource_timeseries.log & wait $$PID ); \
		echo "      -> Time-series log saved to $(BUILD_DIR)/resource_timeseries.log"; \
	else \
		echo "      -> 'pidstat' not found. Running program without time-series log."; \
		echo "      -> To install (Arch): sudo pacman -S sysstat"; \
		@$(BUILD_DIR)/$(TARGET); \
	fi

	@echo "\n[2/4] Generating gprof reports from run data..."
	@gprof $(BUILD_DIR)/$(TARGET) gmon.out > $(BUILD_DIR)/profile.txt
	@gprof $(BUILD_DIR)/$(TARGET) gmon.out | gprof2dot | dot -Tpng -o $(BUILD_DIR)/callgraph.png
	@echo "      -> gprof report and call graph saved in $(BUILD_DIR)/"

	@echo "\n[3/4] Re-running to generate resource usage summary..."
	@( /usr/bin/time -v $(BUILD_DIR)/$(TARGET) ) >/dev/null 2> $(BUILD_DIR)/resource_summary.txt
	@echo "      -> Summary (max RAM, CPU %) saved to $(BUILD_DIR)/resource_summary.txt"

	@echo "\n[4/4] Profiling complete."
	@echo "--- Finished ---"

clean:
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ Cleaning up... ]" | tee -a $(LOG_FILE)
	@$(RM) -r $(BUILD_DIR)
	@$(RM) -f gmon.out callgraph.dot
	@$(RM) -f qcs.h test_bundle test_bundle.c

bundle:
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ BUNDLE: Running bundle.py to create qcs.h ]" | tee -a $(LOG_FILE)
	@python bundle.py
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ BUNDLE: qcs.h created successfully ]" | tee -a $(LOG_FILE)
