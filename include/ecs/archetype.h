#pragma once

#include "ecs/config.h"
#include "ecs/collumn.h"
#include "ecs/type_declaration.h"
#include "ecs/component_init.h"
#include "ecs/component_declaration.h"

namespace ecs
{

// namespace details
// {

using Type = ska::flat_hash_map<ComponentId, TypeId  /*ComponentId already has TypeId*/>;

ArchetypeId get_archetype_id(const Type &type);

struct Archetype
{
  Type type;
  ArchetypeId archetypeId;

  std::vector<Collumn> collumns;

  ska::flat_hash_map<ComponentId, size_t> componentToCollumnIndex;

  uint32_t entityCount = 0;
  uint32_t chunkSize = 0;
  uint32_t capacity = 0;
  uint32_t chunkCount = 0;

  Archetype() = default;
  Archetype(const TypeDeclarationMap &type_map, Type &&_type, ArchetypeChunkSize chunk_size_power);

  int get_component_index(ComponentId componentId) const
  {
    auto it = componentToCollumnIndex.find(componentId);
    return it != componentToCollumnIndex.end() ? it->second : -1;
  }

  void try_add_chunk(int requiredEntityCount);

  // return index of the added entity
  void add_entity(const TypeDeclarationMap &type_map, const InitializerList &template_init, InitializerList override_list);

  void add_entities(const TypeDeclarationMap &type_map, const InitializerList &template_init, InitializerSoaList override_soa_list);

  void remove_entity(const TypeDeclarationMap &type_map, uint32_t entityIndex);

};

using ArchetypeMap = ska::flat_hash_map<ArchetypeId, Archetype>;

ArchetypeId get_or_create_archetype(const ComponentDeclarationMap &component_map, const TypeDeclarationMap &type_map, ArchetypeMap &archetype_map, const InitializerList &components, ArchetypeChunkSize chunk_size_power);

// } // namespace details

} // namespace ecs
