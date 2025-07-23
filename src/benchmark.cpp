#include "../include/benchmark.hpp"

/**
 * @brief Records the current time as the starting point.
 */
void Timer::start() { start_time = high_res_clock::now(); }

/**
 * @brief Calculates the elapsed time since start() was called.
 * @return The elapsed time in milliseconds.
 */
double Timer::stop() {
  auto end_time = high_res_clock::now();

  std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

  return elapsed.count();
}
