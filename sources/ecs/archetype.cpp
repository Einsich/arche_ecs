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
  for (const ecs::ComponentId componentId : type)
  {
    id = ecs::fnv_hash(componentId, id);
  }
  return id;
}

Archetype::Archetype(const ecs::EcsManager &mgr, ecs::ArchetypeId archetype_id, ArchetypeComponentType &&_type, ecs::ArchetypeChunkSize chunk_size_power) :
  type(std::move(_type)), archetypeId(archetype_id)
{
  assert(!type.empty());
  collumns.reserve(type.size());
  componentToCollumnIndex.reserve(type.size());
  for (const ecs::ComponentId componentId : type)
  {
    const ecs::TypeId typeId = ecs::get_type_id(componentId);
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(mgr.typeMap, typeId);
    if (typeDeclaration == nullptr)
    {
      ECS_LOG_ERROR(mgr).log("Type with hash %x not found", typeId);
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

void add_entity_to_archetype(Archetype &archetype, ecs::EcsManager &mgr, const ecs::InitializerList &template_init, ecs::InitializerList &&override_list)
{
  try_add_chunk(archetype, 1);
  for (ecs_details::Collumn &collumn : archetype.collumns)
  {
    void *dstData = collumn.get_data(archetype.entityCount);
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(mgr.typeMap, collumn.typeId);
    // firstly check initialization data in override_list and move it
    auto it = override_list.args.find(collumn.componentId);
    if (it != override_list.args.end())
    {
      void *srcData = it->second.data();
      if (srcData != nullptr)
      {
        if (it->second.typeId == typeDeclaration->typeId)
        {
          typeDeclaration->move_construct(dstData, srcData);
          continue;
        }
        else
        {
          const char *componentName = mgr.componentMap.find(it->first)->second->name.c_str();
          const char *receivedType = mgr.typeMap.find(it->second.typeId)->second.typeName.c_str();
          const char *expectedType = mgr.typeMap.find(collumn.typeId)->second.typeName.c_str();
          ECS_LOG_ERROR(mgr).log("Component %s has type %s but expected %s, during create_entity",
            componentName, receivedType, expectedType);
        }
      }
      else
      {
        typeDeclaration->construct_default(dstData);
        continue;
      }
    }
    // then check initialization data in template_init and copy it
    auto it2 = template_init.args.find(collumn.componentId);
    if (it2 != template_init.args.end())
    {
      assert(it2->second.typeId == collumn.typeId);
      typeDeclaration->copy_construct(dstData, it2->second.data());
      continue;
    }
    // if there is no initialization data, construct default
    typeDeclaration->construct_default(dstData);
    // but this is error because we have to have initialization data for all components
    ECS_LOG_ERROR(mgr).log("No initialization data for component %s", collumn.debugName.c_str());
  }
  archetype.entityCount++;
  ecs_details::consume_init_list(mgr, std::move(override_list));
}

void add_entities_to_archetype(Archetype &archetype, ecs::EcsManager &mgr, const ecs::InitializerList &template_init, ecs::InitializerSoaList &&override_soa_list)
{
  int requiredEntityCount = override_soa_list.size();
  try_add_chunk(archetype, requiredEntityCount);
  for (ecs_details::Collumn &collumn : archetype.collumns)
  {
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(mgr.typeMap, collumn.typeId);

    // firstly check initialization data in override_soa_list and move it
    auto it = override_soa_list.args.find(collumn.componentId);
    if (it != override_soa_list.args.end())
    {
      ecs::ComponentDataSoa &componentDataSoa = it->second;
      if (componentDataSoa.typeId == collumn.typeId)
      {
        for (int i = 0; i < requiredEntityCount; i++)
        {
          typeDeclaration->move_construct(collumn.get_data(archetype.entityCount + i), componentDataSoa.get_data(i));
        }
        continue;
      }
      else
      {
        const char *componentName = mgr.componentMap.find(it->first)->second->name.c_str();
        const char *receivedType = mgr.typeMap.find(componentDataSoa.typeId)->second.typeName.c_str();
        const char *expectedType = mgr.typeMap.find(collumn.typeId)->second.typeName.c_str();
        ECS_LOG_ERROR(mgr).log("Component %s has type %s but expected %s, during create_entities",
          componentName, receivedType, expectedType);
      }
    }
    // then check initialization data in template_init and copy it
    auto it2 = template_init.args.find(collumn.componentId);
    if (it2 != template_init.args.end())
    {
      const ecs::Any &componentData = it2->second;
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
    ECS_LOG_ERROR(mgr).log("No initialization data for component %s", collumn.debugName.c_str());
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
    try_registrate(mgr, query, archetypePtr.get());
  }
  for (auto &query : mgr.systems)
  {
    try_registrate(mgr, query, archetypePtr.get());
  }
  for (auto &[id, query] : mgr.events)
  {
    try_registrate(mgr, query, archetypePtr.get());
  }
  mgr.archetypeMap[archetype.archetypeId] = std::move(archetypePtr);
}

ecs::ArchetypeId get_or_create_archetype(ecs::EcsManager &mgr, ecs::InitializerList &components, ecs::ArchetypeChunkSize chunk_size_power, const char *template_name)
{
  ArchetypeComponentType type;
  type.reserve(components.size());

  for (auto it = components.args.begin(); it != components.args.end();)
  {
    auto &arg = *it;
    auto cmpIt = mgr.componentMap.find(arg.first);
    if (cmpIt != mgr.componentMap.end())
    {
      if (cmpIt->second->typeId == arg.second.typeId)
      {
        type.insert(arg.first);
        ++it;
        continue;
      }
      else
      {
        const char *componentName = cmpIt->second->name.c_str();
        const char *receivedType = mgr.typeMap.find(arg.second.typeId)->second.typeName.c_str();
        const char *expectedType = mgr.typeMap.find(cmpIt->second->typeId)->second.typeName.c_str();
        ECS_LOG_ERROR(mgr).log("Component %s has type %s but expected %s, during instantiation template \"%s\"",
          componentName, receivedType, expectedType, template_name);
      }
    }
    else
    {
      const char *receivedType = mgr.typeMap.find(arg.second.typeId)->second.typeName.c_str();
      ECS_LOG_ERROR(mgr).log("Component %llx of type %s not found, during instantiation template \"%s\"", arg.first, receivedType, template_name);
    }

    it = components.args.erase(it);
    continue;
  }

  ecs::ArchetypeId archetypeId = get_archetype_id(type);

  if (mgr.archetypeMap.find(archetypeId) == mgr.archetypeMap.end())
  {
    register_archetype(mgr, Archetype(mgr, archetypeId, std::move(type), chunk_size_power));
  }

  return archetypeId;
}


} // namespace ecs