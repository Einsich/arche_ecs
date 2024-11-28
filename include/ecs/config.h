#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "ecs/ska/flat_hash_map.hpp"

namespace ecs
{
  using ComponentId = uint32_t;
  using TypeId = uint32_t;
  using NameHash = uint32_t;
  using ArchetypeId = uint32_t;
  using TemplateId = uint32_t;

}