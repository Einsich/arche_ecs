#pragma once

#include "ecs/config.h"
#include "ecs/collumn.h"
#include "ecs/type_declaration.h"
#include "ecs/component_init.h"
#include "ecs/component_declaration.h"

namespace ecs_details
{

using ArchetypeComponentType = ska::flat_hash_set<ecs::ComponentId>;

struct Archetype
{
  ArchetypeComponentType type;
  ecs::ArchetypeId archetypeId;

  std::vector<ecs_details::Collumn> collumns;

  ska::flat_hash_map<ecs::ComponentId, size_t> componentToCollumnIndex;

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

};

// return index of the added entity
void add_entity_to_archetype(Archetype &archetype, const ecs::EcsManager &mgr, const ecs::InitializerList &template_init, ecs::InitializerList &&override_list);

void add_entities_to_archetype(Archetype &archetype, const ecs::EcsManager &mgr, const ecs::InitializerList &template_init, ecs::InitializerSoaList &&override_soa_list);

void remove_entity_from_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map, uint32_t entityIndex);

void destroy_all_entities_from_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map);

} // namespace ecs
