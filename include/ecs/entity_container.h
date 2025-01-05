#pragma once
#include "ecs/entity_id.h"

namespace ecs_details
{

  struct EntityRecord
  {
    ecs::ArchetypeId archetypeId;
    uint32_t componentIndex;
    uint32_t generation;
    bool isAlive;
  };

  struct EntityContainer
  {
    std::vector<EntityRecord> entityRecords;
    std::vector<int> freeIndices;

    ecs::EntityId create_entity(ecs::ArchetypeId archetypeId, uint32_t componentIndex)
    {
      ecs::EntityId entityId;
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

    bool is_alive(ecs::EntityId entityId) const
    {
      return entityId.entityIndex < entityRecords.size() && entityRecords[entityId.entityIndex].generation == entityId.generation;
    }

    void destroy_entity(ecs::EntityId entityId)
    {
      if (is_alive(entityId))
      {
        entityRecords[entityId.entityIndex].generation = (entityRecords[entityId.entityIndex].generation + 1) & ecs::EntityId::GENERATIONS_MASK;
        entityRecords[entityId.entityIndex].isAlive = false;
        freeIndices.push_back(entityId.entityIndex);
      }
    }

    bool get(ecs::EntityId entityId, ecs::ArchetypeId &archetypeId, uint32_t &componentIndex) const
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

    bool mutate(ecs::EntityId entityId, ecs::ArchetypeId archetypeId, uint32_t componentIndex)
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