#pragma once

#include "ecs/config.h"
#include "ecs/archetype.h"

namespace ecs
{

using ToComponentMap = std::vector<std::vector<char *> *>;

struct ArchetypeRecord
{
  Archetype *archetype = nullptr;
  // ArchetypeId archetypeId = 0;
  ToComponentMap toComponentIndex;
};

struct Query
{

  enum class ComponentAccess
  {
    READ_COPY,
    READ_ONLY,
    READ_WRITE,
    READ_ONLY_OPTIONAL,
    READ_WRITE_OPTIONAL,
  };

  struct ComponentAccessInfo
  {
    ComponentId componentId;
    ComponentAccess access;
  };

  std::string uniqueName;
  NameHash nameHash;
  std::vector<ComponentAccessInfo> querySignature;

  std::vector<ComponentId> requireComponents; // components without reading access
  std::vector<ComponentId> excludeComponents;

  bool try_registrate(const Archetype *archetype)
  {
    for (ComponentId componentId : requireComponents)
    {
      if (archetype->get_component_index(componentId) == -1)
      {
        return false;
      }
    }

    for (ComponentId componentId : excludeComponents)
    {
      if (archetype->get_component_index(componentId) != -1)
      {
        return false;
      }
    }

    ToComponentMap toComponentIndex;
    toComponentIndex.reserve(querySignature.size());
    for (const ComponentAccessInfo &componentAccessInfo : querySignature)
    {
      int componentIndex = archetype->get_component_index(componentAccessInfo.componentId);
      if (componentIndex == -1 && !(componentAccessInfo.access == ComponentAccess::READ_ONLY_OPTIONAL || componentAccessInfo.access == ComponentAccess::READ_WRITE_OPTIONAL))
      {
        return false;
      }
      toComponentIndex.push_back(componentIndex >= 0 ? (std::vector<char *> *)&(archetype->collumns[componentIndex].chunks) : nullptr);
    }

    archetypesCache.insert({archetype->archetypeId, {(Archetype *)archetype, std::move(toComponentIndex)}});

    return true;
  }

   ska::flat_hash_map<ArchetypeId, ArchetypeRecord> archetypesCache;
};

struct System final : public Query
{
  using SystemUpdateHandler = void (*)(Archetype &archetype, const ToComponentMap &to_archetype_component);
  SystemUpdateHandler update_archetype;
};

struct EventHandler final : public Query
{
  using BroadcastEventHandler = void (*)(Archetype &archetype, const ToComponentMap &to_archetype_component, EventId event_id, const void *event_ptr);
  using UnicastEventHandler = void (*)(Archetype &archetype, const ToComponentMap &to_archetype_component, uint32_t component_idx, EventId event_id, const void *event_ptr);

  std::vector<EventId> eventIds;
  BroadcastEventHandler broadcastEvent;
  UnicastEventHandler unicastEvent;
};

// using BroadcastReadbackHandler = void (*)(Archetype &archetype, const ToComponentMap &to_archetype_component, EventId event_id, void *event_ptr);
// using UnicastReadbackHandler = void (*)(Archetype &archetype, const ToComponentMap &to_archetype_component, EntityId eid, EventId event_id, void *event_ptr);


}