#pragma once
#include "ecs/entity_id.h"

namespace ecs_details
{
  enum class EntityState
  {
    Dead,
    Alive,
    AsyncCreation,
    AsyncDestroy,
  };

  struct EntityRecord
  {
    ecs::ArchetypeId archetypeId;
    uint32_t componentIndex;
    uint32_t generation;
    EntityState entityState;
  };

  struct EntityContainer
  {
    std::vector<EntityRecord> entityRecords;
    std::vector<int> freeIndices;

    ecs::EntityId allocate_entity(EntityState entity_state)
    {
      ecs::EntityId entityId;
      if (freeIndices.empty())
      {
        entityId.entityIndex = entityRecords.size();
        entityId.generation = 0;
        entityRecords.push_back({ecs::ArchetypeId{0}, 0u, entityId.generation, entity_state});
      }
      else
      {
        entityId.entityIndex = freeIndices.back();
        freeIndices.pop_back();
        uint32_t generation = entityRecords[entityId.entityIndex].generation;
        entityId.generation = generation;
        entityRecords[entityId.entityIndex] = {ecs::ArchetypeId{0}, 0u, generation, entity_state};
      }
      return entityId;
    }


    std::vector<ecs::EntityId> allocate_entities(uint32_t count, EntityState entity_state)
    {
      std::vector<ecs::EntityId> entityIds(count);
      for (uint32_t i = 0; i < count; i++)
      {
        const uint32_t generation = 0u;
        entityIds[i].entityIndex = entityRecords.size();
        entityIds[i].generation = generation;
        entityRecords.push_back({ecs::ArchetypeId{0}, 0u, generation, entity_state});
      }
      return entityIds;
    }


    bool can_access(ecs::EntityId entityId) const
    {
      return entityId.entityIndex < entityRecords.size() &&
          entityRecords[entityId.entityIndex].generation == entityId.generation &&
          (entityRecords[entityId.entityIndex].entityState == EntityState::Alive ||
          entityRecords[entityId.entityIndex].entityState == EntityState::AsyncDestroy);
    }

    bool is_alive(ecs::EntityId entityId) const
    {
      return entityId.entityIndex < entityRecords.size() &&
          entityRecords[entityId.entityIndex].generation == entityId.generation &&
          (entityRecords[entityId.entityIndex].entityState == EntityState::Alive ||
          entityRecords[entityId.entityIndex].entityState == EntityState::AsyncCreation);
    }

    void destroy_entity(ecs::EntityId entityId)
    {
      if (is_alive(entityId))
      {
        entityRecords[entityId.entityIndex].generation = (entityRecords[entityId.entityIndex].generation + 1) & ecs::EntityId::GENERATIONS_MASK;
        entityRecords[entityId.entityIndex].entityState = EntityState::Dead;
        freeIndices.push_back(entityId.entityIndex);
      }
    }

    bool mark_as_destroyed(ecs::EntityId entityId)
    {
      if (is_alive(entityId))
      {
        if (entityRecords[entityId.entityIndex].entityState == EntityState::Alive)
        {
          entityRecords[entityId.entityIndex].entityState = EntityState::AsyncDestroy;
        }
        else // entityState == EntityState::AsyncCreation
        {
          entityRecords[entityId.entityIndex].entityState = EntityState::Dead;
        }
        return true;
      }
      return false;
    }

    bool get(ecs::EntityId entityId, ecs::ArchetypeId &archetypeId, uint32_t &componentIndex) const
    {
      if (can_access(entityId))
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
        entityRecords[entityId.entityIndex].entityState = EntityState::Alive;
        return true;
      }
      return false;
    }
  };
}