#include "ecs/archetype.h"
#include "ecs/ecs_manager.h"
#include "ecs/builtin_events.h"
#include <assert.h>

namespace ecs
{
void perform_event_immediate(EcsManager &mgr, ArchetypeId archetypeId, uint32_t componentIdx, NameHash query_id, EventId event_id, const void *event_ptr);

}
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
  for (const auto [componentId, tracked] : type)
  {
    id = ecs::fnv_hash(componentId, id);
    id = ecs::fnv_hash((uint32_t)tracked, id);
  }
  return id;
}

Archetype::Archetype(const ecs::EcsManager &mgr, ecs::ArchetypeId archetype_id, ArchetypeComponentType &&_type, ecs::ArchetypeChunkSize chunk_size_power) :
  type(std::move(_type)),
  archetypeId(archetype_id),
  chunkSize(1 << chunk_size_power),
  chunkSizePower(chunk_size_power),
  chunkMask(chunkSize - 1)
{
  assert(!type.empty());
  collumns.reserve(type.size());
  componentToCollumnIndex.reserve(type.size());
  for (const auto [componentId, isTracked] : type)
  {
    const ecs::TypeId typeId = ecs::get_type_id(componentId);
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(mgr.typeMap, typeId);
    if (typeDeclaration == nullptr)
    {
      ECS_LOG_ERROR(mgr).log("Type with hash %x not found", typeId);
      continue;
    }
    uint32_t componentIndex = collumns.size();
    collumns.emplace_back(chunk_size_power, typeDeclaration->sizeOfElement, typeDeclaration->alignmentOfElement, typeId, typeDeclaration->typeName.c_str(), componentId);

    componentToCollumnIndex.emplace(componentId, componentIndex);

    if (isTracked)
    {
      uint32_t trackedComponentIndex = componentToTrackedCollumnIndex.size();
      componentToTrackedCollumnIndex.emplace(componentId, trackedComponentIndex);
      trackedCollumns.emplace_back(chunk_size_power, typeDeclaration->sizeOfElement, typeDeclaration->alignmentOfElement, typeId, typeDeclaration->typeName.c_str(), componentId, componentIndex);
    }
  }
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
    }
    for (ecs_details::TrackedCollumn &trackedCollumn : archetype.trackedCollumns)
    {
      trackedCollumn.add_chunk();
      trackedCollumn.dirtyState.resize(archetype.capacity, false);
    }
  }
}

void add_entity_to_archetype(Archetype &archetype, ecs::EcsManager &mgr, const ecs::InitializerList &template_init, ecs::InitializerList &&override_list)
{
  try_add_chunk(archetype, 1);
  for (ecs_details::Collumn &collumn : archetype.collumns)
  {
    void *dstData = archetype.getData(collumn, archetype.entityCount);
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

  for (ecs_details::TrackedCollumn &trackedCollumn : archetype.trackedCollumns)
  {
    const ecs_details::Collumn &collumn = archetype.collumns[trackedCollumn.collumnIdx];
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(mgr.typeMap, collumn.typeId);
    const void *srcData = archetype.getData(collumn, archetype.entityCount);
    void *trackedData = archetype.getData(trackedCollumn, archetype.entityCount);
    typeDeclaration->copy_construct(trackedData, srcData);
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
          typeDeclaration->move_construct(archetype.getData(collumn, archetype.entityCount + i), componentDataSoa.get_data(i));
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
        typeDeclaration->copy_construct(archetype.getData(collumn, archetype.entityCount + i), componentData.data());
      }
      continue;
    }
    // if there is no initialization data, construct default
    for (int i = 0; i < requiredEntityCount; i++)
    {
      typeDeclaration->construct_default(archetype.getData(collumn, archetype.entityCount + i));
    }
    // but this is error because we have to have initialization data for all components
    ECS_LOG_ERROR(mgr).log("No initialization data for component %s", collumn.debugName.c_str());
  }

  for (ecs_details::TrackedCollumn &trackedCollumn : archetype.trackedCollumns)
  {
    const ecs_details::Collumn &collumn = archetype.collumns[trackedCollumn.collumnIdx];
    const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(mgr.typeMap, collumn.typeId);

    for (int i = 0; i < requiredEntityCount; i++)
    {
      const void *srcData = archetype.getData(collumn, archetype.entityCount + i);
      void *trackedData = archetype.getData(trackedCollumn, archetype.entityCount + i);
      typeDeclaration->copy_construct(trackedData, srcData);
    }
  }

  archetype.entityCount += requiredEntityCount;
}

static void remove_entity_from_archetype_collumn(Archetype &archetype, ecs_details::Collumn &collumn, const ecs::TypeDeclarationMap &type_map, uint32_t entityIndex)
{
  const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(type_map, collumn.typeId);
  void *removedEntityComponentPtr = archetype.getData(collumn, entityIndex);
  typeDeclaration->destruct(removedEntityComponentPtr);
  if (entityIndex != archetype.entityCount - 1)
  {
    void *lastEntityComponentPtr = archetype.getData(collumn, archetype.entityCount - 1);
    typeDeclaration->move_construct(removedEntityComponentPtr, lastEntityComponentPtr);
  }
}

void remove_entity_from_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map, uint32_t entityIndex)
{
  for (ecs_details::Collumn &collumn : archetype.collumns)
  {
    remove_entity_from_archetype_collumn(archetype, collumn, type_map, entityIndex);
  }
  for (ecs_details::TrackedCollumn &collumn : archetype.trackedCollumns)
  {
    remove_entity_from_archetype_collumn(archetype, collumn, type_map, entityIndex);
    if (entityIndex != archetype.entityCount - 1)
    {
      const bool isDirty = collumn.dirtyState[archetype.entityCount - 1];
      collumn.dirtyState[entityIndex] = isDirty;
    }
  }
  archetype.entityCount--;
}

static void destroy_all_entities_from_archetype_collumn(Archetype &archetype, ecs_details::Collumn &collumn, const ecs::TypeDeclarationMap &type_map)
{
  const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(type_map, collumn.typeId);
  for (uint32_t i = 0; i < archetype.entityCount; i++)
  {
    void *entityComponentPtr = archetype.getData(collumn, i);
    typeDeclaration->destruct(entityComponentPtr);
  }
}

void destroy_all_entities_from_archetype(Archetype &archetype, const ecs::TypeDeclarationMap &type_map)
{
  for (ecs_details::Collumn &collumn : archetype.collumns)
  {
    destroy_all_entities_from_archetype_collumn(archetype, collumn, type_map);
  }
  for (ecs_details::TrackedCollumn &collumn : archetype.trackedCollumns)
  {
    destroy_all_entities_from_archetype_collumn(archetype, collumn, type_map);
  }
  archetype.entityCount = 0;
}

bool try_registrate_track(ecs::EcsManager &mgr, const std::vector<ecs::ComponentId> &tracked_components, ecs_details::Archetype &archetype, ecs::NameHash event_hash)
{
  ecs_details::TrackMask mask = 0u;
  for (ecs::ComponentId componentId : tracked_components)
  {
    int componentIndex = archetype.getComponentTrackedCollumnIndex(componentId);
    if (componentIndex != -1)
    {
      const ecs::TypeDeclaration *typeDeclaration = ecs_details::find_type_declaration(mgr.typeMap, ecs::get_type_id(componentId));
      if (!typeDeclaration->compare_and_assign)
      {
        ECS_LOG_ERROR(mgr).log("Type %s has no compare_and_assign function, and can't be tracked", typeDeclaration->typeName.c_str());
        continue;
      }
      assert(componentIndex < ecs_details::MAX_TRACKED_COMPONENTS);
      mask |= 1u << componentIndex;
    }
  }
  if (mask != 0u)
  {
    archetype.trackedEvents.push_back({event_hash, mask});
    return true;
  }
  else
  {
    return false;
  }
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
    if (!query.trackedComponents.empty())
    {
      ecs_details::try_registrate_track(mgr, query.trackedComponents, *archetypePtr, query.nameHash);
    }
  }
  mgr.archetypeMap[archetype.archetypeId] = std::move(archetypePtr);
}

ecs::ArchetypeId get_or_create_archetype(ecs::EcsManager &mgr, ecs::InitializerList &components, const ecs::TrackedComponentMap &tracked_component_map, ecs::ArchetypeChunkSize chunk_size_power, const char *template_name)
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
        const bool isTracked = tracked_component_map.find(ecs::get_component_name_hash(arg.first)) != tracked_component_map.end();
        type.emplace(arg.first, isTracked);
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


void track_changes(ecs::EcsManager &mgr, ecs_details::Archetype &archetype)
{
  if (archetype.trackedEvents.empty() || archetype.trackedCollumns.empty())
    return;

  std::vector<TrackMask> trackMaskPerEntity;
  trackMaskPerEntity.reserve(archetype.entityCount);
  for (uint32_t i = 0; i < archetype.entityCount; i++)
  {
    ecs_details::TrackMask entityMask = 0u;

    for (uint32_t j = 0, n = archetype.trackedCollumns.size(); j < n; j++)
    {
      ecs_details::TrackedCollumn &trackedCollumn = archetype.trackedCollumns[j];
      if (trackedCollumn.dirtyFlags == ecs_details::TrackedCollumn::CLEAN)
        continue;
      const ecs_details::Collumn &collumn = archetype.collumns[trackedCollumn.collumnIdx];

      const ecs::TypeDeclaration *typeDeclaration = find_type_declaration(mgr.typeMap, collumn.typeId);
      assert(typeDeclaration->compare_and_assign != nullptr);
      const char *newComponentPtr = archetype.getData(collumn, i);
      char *oldComponentPtr = archetype.getData(trackedCollumn, i);
      bool changed = typeDeclaration->compare_and_assign(newComponentPtr, oldComponentPtr);
      if (changed)
      {
        assert(j < ecs_details::MAX_TRACKED_COMPONENTS);
        entityMask |= 1u << j;
      }
    }
    trackMaskPerEntity.emplace_back(entityMask);
  }

  for (ecs_details::TrackedCollumn &trackedCollumn : archetype.trackedCollumns)
  {
    trackedCollumn.reset_dirty();
  }

  const ecs::OnTrack event;
  for (uint32_t i = 0, n = trackMaskPerEntity.size(); i < n; i++)
  {
    ecs_details::TrackMask entityMask = trackMaskPerEntity[i];
    if (entityMask != 0u)
    {
      for (const auto &[event_hash, mask] : archetype.trackedEvents)
      {
        if ((entityMask & mask) != 0u)
        {
          ecs::perform_event_immediate(mgr, archetype.archetypeId, i, event_hash, ecs::EventInfo<ecs::OnTrack>::eventId, &event);
        }
      }
    }
  }
}

} // namespace ecs