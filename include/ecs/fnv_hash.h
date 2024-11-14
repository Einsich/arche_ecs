#pragma once
#include <cstdint>

namespace ecs
{
  constexpr uint32_t fnv_hash(uint32_t a, uint32_t h)
  {
    return (h * 16777619u) ^ a;
  }

  constexpr uint32_t fnv_hash(const char *str, uint32_t h)
  {
    const unsigned char *p = (const unsigned char *)str;
    for (; *p; ++p)
      h = (h * 16777619u) ^ *p;
    return h;
  }

  constexpr uint32_t hash(const char *str, uint32_t h = 2166136261u)
  {
    return fnv_hash(str, h);
  }
}