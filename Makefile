SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
PROFILE_DIR = profile
LOG_DIR = log

CC = gcc
CFLAGS = -std=c89 -O3 -march=native -mavx2 -mfma -pg -g -Wall -Wextra -pedantic -I$(INCLUDE_DIR)
LDFLAGS = -lm -pthread -fopenmp -lOpenCL
AR = ar
ARFLAGS = rcs

# Single header approach - include all source files
# Parallelization mode is selected at compile time with preprocessor definitions
# Default: sequential (no parallelization overhead)
# Options: QCS_MULTI_THREAD, QCS_CPU_OPENMP, QCS_GPU_OPENCL, QCS_SIMD_ONLY
PARALLEL_MODE ?= 

# Add parallelization-specific flags
ifeq ($(PARALLEL_MODE),QCS_MULTI_THREAD)
  CFLAGS += -DQCS_MULTI_THREAD
  LDFLAGS += -pthread
endif

ifeq ($(PARALLEL_MODE),QCS_CPU_OPENMP)
  CFLAGS += -DQCS_CPU_OPENMP -fopenmp
  LDFLAGS += -fopenmp
endif

ifeq ($(PARALLEL_MODE),QCS_GPU_OPENCL)
  CFLAGS += -DQCS_GPU_OPENCL
  LDFLAGS += -lOpenCL
endif

ifeq ($(PARALLEL_MODE),QCS_SIMD_ONLY)
  CFLAGS += -DQCS_SIMD_ONLY
endif

LOG_FILE = $(LOG_DIR)/$(TARGET).build_log

# Include all source files - parallelization is handled by preprocessor definitions
LIB_SRCS  := $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))
MAIN_SRC  := $(SRC_DIR)/main.c

LIB_OBJS  := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(LIB_SRCS))
MAIN_OBJ  := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(MAIN_SRC))

LIB = $(BUILD_DIR)/libqcs.a
EXE = $(BUILD_DIR)/qcs

.PHONY: all clean newdir run bundle profile test

all: $(LIB) $(EXE)

$(LIB): $(LIB_OBJS) | newdir
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ AR: $@ ]" | tee -a $(LOG_FILE)
	@$(AR) $(ARFLAGS) $@ $(LIB_OBJS)

$(EXE): $(MAIN_OBJ) $(LIB) | newdir
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ LNK: $@ ]" | tee -a $(LOG_FILE)
	@$(CC) $(MAIN_OBJ) -o $@ -L$(BUILD_DIR) -lqcs $(LDFLAGS)

test: $(LIB)
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ --- Building and Running Test Suite --- ]" | tee -a $(LOG_FILE)
	@$(MAKE) -C test all
	@./test/build/run_tests
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ --- Cleaning up Test Suite --- ]" | tee -a $(LOG_FILE)
	@$(MAKE) -C test clean

run: $(EXE)
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ Running $(EXE) ]" | tee -a $(LOG_FILE)
	@$(EXE) $(ARGS)

profile: $(EXE)
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ PROFILING: Creating profile dir ]" | tee -a $(LOG_FILE)
	@mkdir -p $(PROFILE_DIR)
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ PROFILING: Running profile.py to benchmark and create profiling reports ]" | tee -a $(LOG_FILE)
	@python scripts/profile.py
	@python scripts/plot_all_cpu.py
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ PROFILING: benchmarking and reports generated successfully ]" | tee -a $(LOG_FILE)

bundle:
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ BUNDLE: Running bundle.py to create qcs.h ]" | tee -a $(LOG_FILE)
	@python scripts/bundle.py
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ BUNDLE: qcs.h created successfully ]" | tee -a $(LOG_FILE)

clean:
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ Cleaning up... ]" | tee -a $(LOG_FILE)
	@$(RM) -r $(BUILD_DIR)
	@$(RM) -r $(PROFILE_DIR)
	@$(RM) -r $(LOG_DIR)
	@$(RM) -f gmon.out callgraph.dot
	@$(RM) -f qcs.h test_bundle test_bundle.c
	@$(MAKE) -C test clean

newdir:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(LOG_DIR)
	@touch $(LOG_FILE)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | newdir
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ CC: $< ]" | tee -a $(LOG_FILE)
	@echo "$$(date +'%Y-%m-%d_%H:%M:%S') | [ CFLAGS: $(CFLAGS) ] " | tee -a $(LOG_FILE)
	@$(CC) $(CFLAGS) -c $< -o $@
