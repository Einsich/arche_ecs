#pragma once

#include "ecs/config.h"

namespace ecs
{
  struct EntityId
  {
    static const uint32_t MAX_ENTITIES_COUNT = 1 << 24;
    static const uint32_t MAX_GENERATIONS_COUNT = 1 << 8;
    static const uint32_t GENERATIONS_MASK = (1 << 8) - 1;

    uint32_t entityIndex : 24;
    uint32_t generation : 8;

    EntityId() : entityIndex(MAX_ENTITIES_COUNT - 1), generation(MAX_GENERATIONS_COUNT - 1) {}

    bool operator==(const EntityId &other) const
    {
      return entityIndex == other.entityIndex && generation == other.generation;
    }
    bool operator!=(const EntityId &other) const
    {
      return !(*this == other);
    }
  };

}