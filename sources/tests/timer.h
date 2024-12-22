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

  // return delta time in milliseconds
  float stop()
  {
    float delta = get_time();
    printf("%s : (%f ms)\n", label, delta);
    stopped = true;
    return delta;
  }

  float get_time()
  {
    // print delta time in milliseconds
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::milli>(end - start).count();
  }

  ~Timer()
  {
    if (!stopped)
    {
      stop();
    }
  }
};