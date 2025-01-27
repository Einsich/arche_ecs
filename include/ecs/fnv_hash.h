#pragma once
#include <cstdint>

namespace ecs
{
  constexpr uint32_t fnv_hash(uint32_t a, uint32_t h)
  {
    return (h * 16777619u) ^ a;
  }

  constexpr uint32_t fnv_hash(uint64_t a, uint32_t h)
  {
    h = (h * 16777619u) ^ (a & 0xFFFFFFFF);
    h = (h * 16777619u) ^ (a >> 32);
    return h;
  }

  constexpr uint32_t fnv_hash(const char *str, uint32_t h)
  {
    const char *p = (const char *)str;
    for (; *p; ++p)
      h = (h * 16777619u) ^ (unsigned char)*p;
    return h;
  }

  constexpr uint32_t hash(const char *str, uint32_t h = 2166136261u)
  {
    return fnv_hash(str, h);
  }
}