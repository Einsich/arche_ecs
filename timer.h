#pragma once
#include <iostream>
#include <chrono>

struct Timer
{
  using ns_t = std::chrono::steady_clock::time_point;
  const char *label;
  ns_t start;

  Timer(const char *label) : label(label), start(std::chrono::high_resolution_clock::now()) {}

  ~Timer()
  {
    auto end = std::chrono::high_resolution_clock::now();
    // print delta time in milliseconds
    printf("%s : (%f ms)\n", label, std::chrono::duration<float, std::milli>(end - start).count());
  }
};