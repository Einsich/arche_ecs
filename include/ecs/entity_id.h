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

  struct EntityRecord
  {
    ArchetypeId archetypeId;
    uint32_t componentIndex;
    uint32_t generation;
    bool isAlive;
  };

  struct EntityContainer
  {
    std::vector<EntityRecord> entityRecords;
    std::vector<int> freeIndices;

    EntityId create_entity(ArchetypeId archetypeId, uint32_t componentIndex)
    {
      EntityId entityId;
      if (freeIndices.empty())
      {
        entityId.entityIndex = entityRecords.size();
        entityId.generation = 0;
        entityRecords.push_back({archetypeId, componentIndex, entityId.generation, true});
      }
      else
      {
        entityId.entityIndex = freeIndices.back();
        freeIndices.pop_back();
        uint32_t generation = entityRecords[entityId.entityIndex].generation;
        entityId.generation = generation;
        entityRecords[entityId.entityIndex] = {archetypeId, componentIndex, generation, true};
      }
      return entityId;
    }

    bool is_alive(EntityId entityId) const
    {
      return entityId.entityIndex < entityRecords.size() && entityRecords[entityId.entityIndex].generation == entityId.generation;
    }

    void destroy_entity(EntityId entityId)
    {
      if (is_alive(entityId))
      {
        entityRecords[entityId.entityIndex].generation = (entityRecords[entityId.entityIndex].generation + 1) & EntityId::GENERATIONS_MASK;
        entityRecords[entityId.entityIndex].isAlive = false;
        freeIndices.push_back(entityId.entityIndex);
      }
    }

    bool get(EntityId entityId, ArchetypeId &archetypeId, uint32_t &componentIndex) const
    {
      if (is_alive(entityId))
      {
        const EntityRecord &entityRecord = entityRecords[entityId.entityIndex];
        archetypeId = entityRecord.archetypeId;
        componentIndex = entityRecord.componentIndex;
        return true;
      }
      return false;
    }

    bool mutate(EntityId entityId, ArchetypeId archetypeId, uint32_t componentIndex)
    {
      if (is_alive(entityId))
      {
        entityRecords[entityId.entityIndex].archetypeId = archetypeId;
        entityRecords[entityId.entityIndex].componentIndex = componentIndex;
        return true;
      }
      return false;
    }
  };
}