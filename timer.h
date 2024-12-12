#pragma once
#include <iostream>
#include <chrono>

struct Timer
{
  using ns_t = std::chrono::steady_clock::time_point;
  const char *label;
  ns_t start;
  bool stopped = false;

  Timer(const char *label) : label(label), start(std::chrono::high_resolution_clock::now()) {}

  void stop()
  {
    auto end = std::chrono::high_resolution_clock::now();
    // print delta time in milliseconds
    printf("%s : (%f ns)\n", label, std::chrono::duration<float, std::micro>(end - start).count());
    stopped = true;
  }
  ~Timer()
  {
    if (!stopped)
    {
      stop();
    }
  }
};