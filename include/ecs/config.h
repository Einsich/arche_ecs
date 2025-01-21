#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include "ecs/ska/flat_hash_map.hpp"

namespace ecs
{
  using ComponentId = uint64_t;
  using TypeId = uint32_t;
  using NameHash = uint32_t;
  using ArchetypeId = uint32_t;
  using TemplateId = uint32_t;
  using EventId = uint32_t;

  #define ECS_UNUSED(x) (void)(x)

}