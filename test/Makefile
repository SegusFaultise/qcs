CC = gcc
CFLAGS = -std=c89 -Wall -Wextra -I../include -g
LDFLAGS = -L../build -lqcs -lpthread -lm

BUILD_DIR = build

EXE = $(BUILD_DIR)/run_tests

TEST_SRCS = $(wildcard test_*.c)
TEST_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(TEST_SRCS))

TARGET = $(BUILD_DIR)/run_tests

.PHONY: all clean newdir

all: $(TARGET)

$(TARGET): $(TEST_OBJS) | newdir
	@echo "LNK_TEST: $@"
	@$(CC) $(TEST_OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c | newdir
	@echo "CC_TEST: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

newdir:
	@mkdir -p $(BUILD_DIR)

clean:
	@$(RM) -r $(BUILD_DIR)
