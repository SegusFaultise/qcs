#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <chrono>

/**
 * @struct Timer
 * @brief A simple high-resolution timer for benchmarking code execution.
 */
struct Timer {
  using high_res_clock = std::chrono::high_resolution_clock;
  using time_point = std::chrono::time_point<high_res_clock>;

  time_point start_time;

  void start();

  double stop();
};

#endif // BENCHMARK_HPP
