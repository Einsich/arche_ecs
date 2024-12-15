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
    auto end = std::chrono::high_resolution_clock::now();
    // print delta time in milliseconds
    float delta = std::chrono::duration<float, std::milli>(end - start).count();
    printf("%s : (%f ns)\n", label, delta);
    stopped = true;
    return delta;
  }

  ~Timer()
  {
    if (!stopped)
    {
      stop();
    }
  }
};