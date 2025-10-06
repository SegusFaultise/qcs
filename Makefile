TARGET = qcs
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
LOG_FILE = $(BUILD_DIR)/$(TARGET).build_log

CC = gcc
C_STANDARD = std=c89

CFLAGS = -Wall -Wextra -pedantic -std=c89 -pg -g -I$(INCLUDE_DIR)
LDFLAGS = -lm

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))


.PHONY: all clean newdir run bundle

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
	@$(BUILD_DIR)/$(TARGET)

clean:
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ Cleaning up $(BUILD_DIR) ]" | tee -a $(LOG_FILE)
	@$(RM) -r $(BUILD_DIR)
	@$(RM) -f qcs.h test_bundle test_bundle.c $(BUILD_DIR)

bundle:
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ BUNDLE: Running bundle.py to create qcs.h ]" | tee -a $(LOG_FILE)
	@python bundle.py
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ BUNDLE: qcs.h created successfully ]" | tee -a $(LOG_FILE)
