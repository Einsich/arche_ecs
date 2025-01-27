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
  std::vector<int> toTrackedComponent;
  ArchetypeRecord() = default;
  ArchetypeRecord(ecs_details::Archetype *archetype, ToComponentMap &&toComponentIndex, std::vector<int> &&toTrackedComponent) :
      archetype(archetype), toComponentIndex(std::move(toComponentIndex)), toTrackedComponent(std::move(toTrackedComponent)) {}
};

void mark_dirty(ecs_details::Archetype &archetype, const std::vector<int> &to_tracked_component, uint32_t component_idx);
void mark_dirty(ecs_details::Archetype &archetype, const std::vector<int> &to_tracked_component);

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

  ska::flat_hash_map<ArchetypeId, ArchetypeRecord> archetypesCache;

  std::vector<ComponentAccessInfo> querySignature;

  std::vector<ComponentId> requireComponents; // components without reading access
  std::vector<ComponentId> excludeComponents;

  std::vector<ecs_details::tiny_string> before;
  std::vector<ecs_details::tiny_string> after;

  ecs_details::tiny_string name;
  ecs_details::tiny_string uniqueName;
  NameHash nameHash;

};

// static_assert(sizeof(Query) == 184);
// static_assert(sizeof(Query::archetypesCache) == 40);
// static_assert(sizeof(Query::before) == 24);

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
  std::vector<ComponentId> trackedComponents;
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