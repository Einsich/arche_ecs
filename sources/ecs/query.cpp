#include "ecs/query.h"
#include "ecs/ecs_manager.h"

namespace ecs
{

bool try_registrate(ecs::Query &query, const ecs_details::Archetype *archetype)
{
  for (ComponentId componentId : query.requireComponents)
  {
    if (archetype->getComponentCollumnIndex(componentId) == -1)
    {
      return false;
    }
  }

  for (ComponentId componentId : query.excludeComponents)
  {
    if (archetype->getComponentCollumnIndex(componentId) != -1)
    {
      return false;
    }
  }

  ToComponentMap toComponentIndex;
  toComponentIndex.reserve(query.querySignature.size());
  for (const Query::ComponentAccessInfo &componentAccessInfo : query.querySignature)
  {
    int componentIndex = archetype->getComponentCollumnIndex(componentAccessInfo.componentId);
    if (componentIndex == -1 && !(componentAccessInfo.access == Query::ComponentAccess::READ_ONLY_OPTIONAL || componentAccessInfo.access == Query::ComponentAccess::READ_WRITE_OPTIONAL))
    {
      return false;
    }
    toComponentIndex.push_back(componentIndex >= 0 ? (std::vector<char *> *)&(archetype->collumns[componentIndex].chunks) : nullptr);
  }

  query.archetypesCache.insert({archetype->archetypeId, {(ecs_details::Archetype *)archetype, std::move(toComponentIndex)}});

  return true;
}

void register_query(EcsManager &mgr, Query &&query)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    try_registrate(query, archetype.get());
  }
  printf("[ECS] Register query %s\n", query.uniqueName.c_str());
  mgr.queries[query.nameHash] = std::move(query);
}

void register_system(EcsManager &mgr, System &&system)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    try_registrate(system, archetype.get());
  }
  printf("[ECS] Register system %s\n", system.uniqueName.c_str());
  mgr.systems[system.nameHash] = std::move(system);
}


void register_event(EcsManager &mgr, EventHandler &&event)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    try_registrate(event, archetype.get());
  }
  printf("[ECS] Register system %s\n", event.uniqueName.c_str());
  for (EventId eventId : event.eventIds)
  {
    mgr.eventIdToHandlers[eventId].push_back(event.nameHash);
  }
  mgr.events[event.nameHash] = std::move(event);
}

void perform_system(const System &system)
{
  for (const auto &[archetypeId, archetypeRecord] : system.archetypesCache)
  {
    system.update_archetype(*archetypeRecord.archetype, archetypeRecord.toComponentIndex);
  }
}

}