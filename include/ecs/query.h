#pragma once

#include "ecs/config.h"
#include "ecs/archetype.h"

namespace ecs
{

using ToComponentMap = std::vector<std::vector<char *> *>;

struct ArchetypeRecord
{
  ecs_details::Archetype *archetype = nullptr;
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

  std::string name;
  std::string uniqueName;
  NameHash nameHash;
  std::vector<ComponentAccessInfo> querySignature;

  std::vector<ComponentId> requireComponents; // components without reading access
  std::vector<ComponentId> excludeComponents;

  std::vector<std::string> before;
  std::vector<std::string> after;

  ska::flat_hash_map<ArchetypeId, ArchetypeRecord> archetypesCache;
};

struct System final : public Query
{
  using SystemUpdateHandler = void (*)(ecs_details::Archetype &archetype, const ToComponentMap &to_archetype_component);
  SystemUpdateHandler update_archetype;
};

struct EventHandler final : public Query
{
  using BroadcastEventHandler = void (*)(ecs_details::Archetype &archetype, const ToComponentMap &to_archetype_component, EventId event_id, const void *event_ptr);
  using UnicastEventHandler = void (*)(ecs_details::Archetype &archetype, const ToComponentMap &to_archetype_component, uint32_t component_idx, EventId event_id, const void *event_ptr);

  std::vector<EventId> eventIds;
  BroadcastEventHandler broadcastEvent;
  UnicastEventHandler unicastEvent;
};

// using BroadcastReadbackHandler = void (*)(ecs_details::Archetype &archetype, const ToComponentMap &to_archetype_component, EventId event_id, void *event_ptr);
// using UnicastReadbackHandler = void (*)(ecs_details::Archetype &archetype, const ToComponentMap &to_archetype_component, EntityId eid, EventId event_id, void *event_ptr);

struct EcsManager;

void register_query(EcsManager &mgr, Query &&query);
void register_system(EcsManager &mgr, System &&system);
void register_event(EcsManager &mgr, EventHandler &&event);

bool try_registrate(ecs::EcsManager &mgr, ecs::Query &query, const ecs_details::Archetype *archetype);

//helper function
void perform_system(const System &system);

}