#include "ecs/archetype.h"
#include <assert.h>


namespace ecs
{

// namespace details
// {

ArchetypeId get_archetype_id(const Type &type)
{
  uint32_t id = 0;
  for (const auto &[componentId, typeId]  : type)
  {
    id = fnv_hash(id, componentId);
  }
  return id;
}

Archetype::Archetype(const TypeDeclarationMap &type_map, Type &&_type, ArchetypeChunkSize chunk_size_power) : type(std::move(_type))
{
  assert(!type.empty());
  archetypeId = get_archetype_id(type);
  collumns.reserve(type.size());
  componentToCollumnIndex.reserve(type.size());
  for (auto [componentId, typeId] : type)
  {
    const TypeDeclaration *typeDeclaration = type_map.find(typeId)->second.get();
    collumns.emplace_back(chunk_size_power, typeDeclaration->sizeOfElement, typeDeclaration->alignmentOfElement, typeId);
    Collumn &collumn = collumns.back();
    collumn.debugName = typeDeclaration->typeName;
    collumn.componentId = componentId;
    componentToCollumnIndex[componentId] = collumns.size() - 1;
  }
  chunkSize = collumns[0].chunkSize;
}

void Archetype::try_add_chunk(int requiredEntityCount)
{
  while (entityCount + requiredEntityCount > capacity)
  {
    capacity += chunkSize;
    chunkCount++;
    for (Collumn &collumn : collumns)
    {
      collumn.add_chunk();
      assert(capacity == collumn.capacity);
    }
  }
}

void Archetype::add_entity(const TypeDeclarationMap &type_map, const InitializerList &template_init, InitializerList override_list)
{
  try_add_chunk(1);
  for (Collumn &collumn : collumns)
  {

    const TypeDeclaration *typeDeclaration = type_map.find(collumn.typeId)->second.get();
    // firstly check initialization data in override_list and move it
    auto it = override_list.args.find(collumn.componentId);
    if (it != override_list.args.end())
    {
      typeDeclaration->move_construct(collumn.get_data(entityCount), it->second.data);
      continue;
    }
    // then check initialization data in template_init and copy it
    auto it2 = template_init.args.find(collumn.componentId);
    if (it2 != template_init.args.end())
    {
      typeDeclaration->copy_construct(collumn.get_data(entityCount), it2->second.data);
      continue;
    }
    // if there is no initialization data, construct default
    typeDeclaration->construct_default(collumn.get_data(entityCount));
    // but this is error because we have to have initialization data for all components
    printf("[ECS] Error: no initialization data for component %s\n", collumn.debugName.c_str());
  }
  entityCount++;
}

void Archetype::add_entities(const TypeDeclarationMap &type_map, const InitializerList &template_init, InitializerSoaList override_soa_list)
{
  int requiredEntityCount = override_soa_list.size();
  try_add_chunk(requiredEntityCount);
  for (Collumn &collumn : collumns)
  {
    const TypeDeclaration *typeDeclaration = type_map.find(collumn.typeId)->second.get();

    // firstly check initialization data in override_soa_list and move it
    auto it = override_soa_list.args.find(collumn.componentId);
    if (it != override_soa_list.args.end())
    {
      ComponentDataSoa &componentDataSoa = it->second;
      for (int i = 0; i < requiredEntityCount; i++)
      {
        typeDeclaration->move_construct(collumn.get_data(entityCount + i), componentDataSoa.data[i]);
      }
      continue;
    }
    // then check initialization data in template_init and copy it
    auto it2 = template_init.args.find(collumn.componentId);
    if (it2 != template_init.args.end())
    {
      const ComponentData &componentData = it2->second;
      for (int i = 0; i < requiredEntityCount; i++)
      {
        typeDeclaration->copy_construct(collumn.get_data(entityCount + i), componentData.data);
      }
      continue;
    }
    // if there is no initialization data, construct default
    for (int i = 0; i < requiredEntityCount; i++)
    {
      typeDeclaration->construct_default(collumn.get_data(entityCount + i));
    }
    // but this is error because we have to have initialization data for all components
    printf("[ECS] Error: no initialization data for component %s\n", collumn.debugName.c_str());
  }
  entityCount += requiredEntityCount;
}

void Archetype::remove_entity(const TypeDeclarationMap &type_map, uint32_t entityIndex)
{
  for (Collumn &collumn : collumns)
  {
    const TypeDeclaration *typeDeclaration = type_map.find(collumn.typeId)->second.get();
    void *removedEntityComponentPtr = collumn.get_data(entityIndex);
    typeDeclaration->destruct(removedEntityComponentPtr);
    if (entityIndex != entityCount - 1)
    {
      void *lastEntityComponentPtr = collumn.get_data(entityCount - 1);
      typeDeclaration->move_construct(removedEntityComponentPtr, lastEntityComponentPtr);
    }
  }
  entityCount--;
}


ArchetypeId get_or_create_archetype(const ComponentDeclarationMap &component_map, const TypeDeclarationMap &type_map, ArchetypeMap &archetype_map, const InitializerList &components, ArchetypeChunkSize chunk_size_power)
{
  Type type;
  type.reserve(components.size() + 1);
  for (const auto &[componentId, component] : components.args)
  {
    type[componentId] = component_map.find(componentId)->second->typeId;
  }

  ArchetypeId archetypeId = get_archetype_id(type);

  if (archetype_map.find(archetypeId) == archetype_map.end())
  {
    archetype_map[archetypeId] = Archetype(type_map, std::move(type), chunk_size_power);
  }

  return archetypeId;
}


// } // namespace details

} // namespace ecs