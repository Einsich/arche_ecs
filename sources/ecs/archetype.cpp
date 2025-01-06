#include "ecs/archetype.h"
#include "ecs/ecs_manager.h"
#include <assert.h>


namespace ecs_details
{

static const ecs::TypeDeclaration *find_type_declaration(const ecs::TypeDeclarationMap &type_map, ecs::TypeId type_id)
{
  const auto it = type_map.find(type_id);
  return it != type_map.end() ? &it->second : nullptr;
}

static ecs::ArchetypeId get_archetype_id(const ArchetypeComponentType &type)
{
  uint32_t id = 0;
  for (const auto &[componentId, typeId]  : type)
  {
    id = ecs::fnv_hash(id, componentId);
  }
  return id;
}

Archetype::Archetype(const ecs::TypeDeclarationMap &type_map, ecs::ArchetypeId archetype_id, ArchetypeComponentType &&_type, ecs::ArchetypeChunkSize chunk_size_power) :
  type(std::move(_type)), archetypeId(archetype_id)
{
  assert(!type.empty());
  collumns.reserve(type.size());
  componentToCollumnIndex.reserve(type.size());
  for (auto [componentId, typeId] : type)
  {
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(type_map, typeId);
    if (typeDeclaration == nullptr)
    {
      printf("[ECS] Error: Type not found\n");
      continue;
    }
    collumns.emplace_back(chunk_size_power, typeDeclaration->sizeOfElement, typeDeclaration->alignmentOfElement, typeId);
    ecs_details::Collumn &collumn = collumns.back();
    collumn.debugName = typeDeclaration->typeName;
    collumn.componentId = componentId;
    componentToCollumnIndex[componentId] = collumns.size() - 1;
  }
  chunkSize = collumns[0].chunkSize;
  chunkSizePower = collumns[0].chunkSizePower;
  chunkMask = collumns[0].chunkMask;
}

static void try_add_chunk(Archetype &archetype, int requiredEntityCount)
{
  while (archetype.entityCount + requiredEntityCount > archetype.capacity)
  {
    archetype.capacity += archetype.chunkSize;
    archetype.chunkCount++;
    for (ecs_details::Collumn &collumn : archetype.collumns)
    {
      collumn.add_chunk();
      assert(archetype.capacity == collumn.capacity);
    }
  }
}

void add_entity_to_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map, const ecs::InitializerList &template_init, ecs::InitializerList &&override_list)
{
  try_add_chunk(archetype, 1);
  for (ecs_details::Collumn &collumn : archetype.collumns)
  {

    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(type_map, collumn.typeId);
    // firstly check initialization data in override_list and move it
    auto it = override_list.args.find(collumn.componentId);
    if (it != override_list.args.end())
    {
      typeDeclaration->move_construct(collumn.get_data(archetype.entityCount), it->second.data());
      continue;
    }
    // then check initialization data in template_init and copy it
    auto it2 = template_init.args.find(collumn.componentId);
    if (it2 != template_init.args.end())
    {
      typeDeclaration->copy_construct(collumn.get_data(archetype.entityCount), it2->second.data());
      continue;
    }
    // if there is no initialization data, construct default
    typeDeclaration->construct_default(collumn.get_data(archetype.entityCount));
    // but this is error because we have to have initialization data for all components
    printf("[ECS] Error: no initialization data for component %s\n", collumn.debugName.c_str());
  }
  archetype.entityCount++;
}

void add_entities_to_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map, const ecs::InitializerList &template_init, ecs::InitializerSoaList &&override_soa_list)
{
  int requiredEntityCount = override_soa_list.size();
  try_add_chunk(archetype, requiredEntityCount);
  for (ecs_details::Collumn &collumn : archetype.collumns)
  {
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(type_map, collumn.typeId);

    // firstly check initialization data in override_soa_list and move it
    auto it = override_soa_list.args.find(collumn.componentId);
    if (it != override_soa_list.args.end())
    {
      ecs::ComponentDataSoa &componentDataSoa = it->second;
      for (int i = 0; i < requiredEntityCount; i++)
      {
        typeDeclaration->move_construct(collumn.get_data(archetype.entityCount + i), componentDataSoa.get_data(i));
      }
      continue;
    }
    // then check initialization data in template_init and copy it
    auto it2 = template_init.args.find(collumn.componentId);
    if (it2 != template_init.args.end())
    {
      const ecs::ComponentData &componentData = it2->second;
      for (int i = 0; i < requiredEntityCount; i++)
      {
        typeDeclaration->copy_construct(collumn.get_data(archetype.entityCount + i), componentData.data());
      }
      continue;
    }
    // if there is no initialization data, construct default
    for (int i = 0; i < requiredEntityCount; i++)
    {
      typeDeclaration->construct_default(collumn.get_data(archetype.entityCount + i));
    }
    // but this is error because we have to have initialization data for all components
    printf("[ECS] Error: no initialization data for component %s\n", collumn.debugName.c_str());
  }
  archetype.entityCount += requiredEntityCount;
}

void remove_entity_from_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map, uint32_t entityIndex)
{
  for (ecs_details::Collumn &collumn : archetype.collumns)
  {
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(type_map, collumn.typeId);
    void *removedEntityComponentPtr = collumn.get_data(entityIndex);
    typeDeclaration->destruct(removedEntityComponentPtr);
    if (entityIndex != archetype.entityCount - 1)
    {
      void *lastEntityComponentPtr = collumn.get_data(archetype.entityCount - 1);
      typeDeclaration->move_construct(removedEntityComponentPtr, lastEntityComponentPtr);
    }
  }
  archetype.entityCount--;
}

void destroy_all_entities_from_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map)
{
  for (ecs_details::Collumn &collumn : archetype.collumns)
  {
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(type_map, collumn.typeId);
    for (uint32_t i = 0; i < archetype.entityCount; i++)
    {
      void *entityComponentPtr = collumn.get_data(i);
      typeDeclaration->destruct(entityComponentPtr);
    }
  }
  archetype.entityCount = 0;
}

static void register_archetype(ecs::EcsManager &mgr, ecs_details::Archetype &&archetype)
{
  std::unique_ptr<ecs_details::Archetype> archetypePtr = std::make_unique<ecs_details::Archetype>(std::move(archetype));
  for (auto &[id, query] : mgr.queries)
  {
    try_registrate(query, archetypePtr.get());
  }
  for (auto &query : mgr.systems)
  {
    try_registrate(query, archetypePtr.get());
  }
  for (auto &[id, query] : mgr.events)
  {
    try_registrate(query, archetypePtr.get());
  }
  mgr.archetypeMap[archetype.archetypeId] = std::move(archetypePtr);
}

ecs::ArchetypeId get_or_create_archetype(ecs::EcsManager &mgr, const ecs::InitializerList &components, ecs::ArchetypeChunkSize chunk_size_power, const char *template_name)
{
  ArchetypeComponentType type;
  type.reserve(components.size() + 1);
  for (const auto &[componentId, component] : components.args)
  {
    auto it = mgr.componentMap.find(componentId);
    if (it != mgr.componentMap.end())
    {
      type[componentId] = it->second->typeId;
    }
    else
    {
      printf("[ECS] Error: Component %x not found, during instantiation template \"%s\"\n", componentId, template_name);
      continue;
    }
  }

  ecs::ArchetypeId archetypeId = get_archetype_id(type);

  if (mgr.archetypeMap.find(archetypeId) == mgr.archetypeMap.end())
  {
    register_archetype(mgr, Archetype(mgr.typeMap, archetypeId, std::move(type), chunk_size_power));
  }

  return archetypeId;
}


} // namespace ecs