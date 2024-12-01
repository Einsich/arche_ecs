#pragma once
#include <time.h>
#include <stdio.h>
#include <ctime>

struct Timer
{
  using ns_t = unsigned long long;

  ns_t start;
  Timer() : start(get_ms()) {}

  ns_t get_time()
  {
    return get_ms() - start;
  }

private:
  static ns_t get_ms()
  {
    std::timespec ts;
    std::timespec_get(&ts, TIME_UTC);
    return ts.tv_nsec / 1000'000u + ts.tv_sec * 1000u;
  }
};