#pragma once
#include <string.h> // for strdup
#include <stdlib.h> // for free
#include "ecs/fnv_hash.h"

namespace ecs_details
{
  // this string stucture has simplified api and is just a wrapper around c-string to reduce sizeof.
  struct tiny_string
  {
    private:
      char *str = nullptr;
    public:
      tiny_string() = default;
      tiny_string(const char *str) : str(strdup(str)) {}
      tiny_string(const tiny_string &other) : str(other.str ? strdup(other.str) : nullptr) {}
      tiny_string(tiny_string &&other) : str(other.str) { other.str = nullptr; }
      ~tiny_string() { if (str) free(str); }
      tiny_string &operator=(const tiny_string &other) { if (str) free(str); str = other.str ? strdup(other.str) : nullptr; return *this; }
      tiny_string &operator=(tiny_string &&other) { if (str) free(str); str = other.str; other.str = nullptr; return *this; }
      const char *c_str() const { return str; }
      // size actually call strlen, there is no cached size
      size_t size() const { return str ? strlen(str) : 0; }

      bool operator==(const tiny_string &other) const { return strcmp(str, other.str) == 0; }
  };
}
namespace std
{

  template<>
  struct hash<ecs_details::tiny_string>
  {
    size_t operator()(const ecs_details::tiny_string &str) const
    {
      return ecs::hash(str.c_str());
    }
  };
}