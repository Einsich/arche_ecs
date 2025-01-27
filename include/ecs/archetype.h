#pragma once

#include "ecs/config.h"
#include "ecs/collumn.h"
#include "ecs/type_declaration.h"
#include "ecs/component_init.h"
#include "ecs/component_declaration.h"

namespace ecs_details
{

using ArchetypeComponentType = ska::flat_hash_map<ecs::ComponentId, bool /*tracked*/>;

struct Archetype
{
  ArchetypeComponentType type;
  ecs::ArchetypeId archetypeId;

  std::vector<ecs_details::Collumn> collumns;
  std::vector<ecs_details::TrackedCollumn> trackedCollumns;
  using TrackedEvent = std::pair<ecs::NameHash, ecs_details::TrackMask>;
  std::vector<TrackedEvent> trackedEvents;

  ska::flat_hash_map<ecs::ComponentId, int32_t> componentToCollumnIndex;
  ska::flat_hash_map<ecs::ComponentId, int32_t> componentToTrackedCollumnIndex;

  uint32_t entityCount = 0;
  uint32_t chunkSize = 0;
  uint32_t chunkSizePower = 0;
  uint32_t chunkMask = 0;
  uint32_t capacity = 0;
  uint32_t chunkCount = 0;

  Archetype() = default;
  Archetype(const ecs::EcsManager &mgr, ecs::ArchetypeId archetype_id, ArchetypeComponentType &&_type, ecs::ArchetypeChunkSize chunk_size_power);

  int getComponentCollumnIndex(ecs::ComponentId componentId) const
  {
    auto it = componentToCollumnIndex.find(componentId);
    return it != componentToCollumnIndex.end() ? it->second : -1;
  }

  int getComponentTrackedCollumnIndex(ecs::ComponentId componentId) const
  {
    auto it = componentToTrackedCollumnIndex.find(componentId);
    return it != componentToTrackedCollumnIndex.end() ? it->second : -1;
  }

  char *getData(ecs_details::Collumn &collumn, uint32_t linear_index) const
  {
    return collumn.chunks[linear_index >> chunkSizePower] + (linear_index & chunkMask) * collumn.sizeOfElement;
  }

  const char *getData(const ecs_details::Collumn &collumn, uint32_t linear_index) const
  {
    return collumn.chunks[linear_index >> chunkSizePower] + (linear_index & chunkMask) * collumn.sizeOfElement;
  }
};

// return index of the added entity
void add_entity_to_archetype(Archetype &archetype, ecs::EcsManager &mgr, const ecs::InitializerList &template_init, ecs::InitializerList &&override_list);

void add_entities_to_archetype(Archetype &archetype, ecs::EcsManager &mgr, const ecs::InitializerList &template_init, ecs::InitializerSoaList &&override_soa_list);

void remove_entity_from_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map, uint32_t entityIndex);

void destroy_all_entities_from_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map);

} // namespace ecs
