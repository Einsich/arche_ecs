#pragma once
#include <cstdint>

namespace ecs
{
  constexpr uint32_t fnv_hash(const char *str)
  {
    uint32_t h = 2166136261u;
    const unsigned char *p = (const unsigned char *)str;
    for (; *p; ++p)
      h = (h * 16777619u) ^ *p;
    return h;
  }

  constexpr uint32_t hash(const char *str)
  {
    return fnv_hash(str);
  }
}