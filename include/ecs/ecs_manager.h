#pragma once

#include "ecs/config.h"
#include "ecs/type_declaration.h"
#include "ecs/component_declaration.h"
#include "ecs/entity_id.h"
#include "ecs/archetype.h"
#include "ecs/template.h"
#include "ecs/query.h"
#include "ecs/event.h"

#include <assert.h>
#include <span>

ECS_TYPE_DECLARATION_ALIAS(ecs::EntityId, "EntityId")
namespace ecs
{
using TemplatesMap = ska::flat_hash_map<TemplateId, Template>;

struct EcsManager
{
  struct DelayedEvent
  {
    ComponentData eventData;
    EventId eventId;
    EntityId entityId;
    bool broadcastEvent;
  };

  TypeDeclarationMap typeMap;
  ComponentDeclarationMap componentMap;
  ArchetypeMap archetypeMap;
  ska::flat_hash_map<NameHash, Query> queries;
  ska::flat_hash_map<NameHash, System> systems;
  ska::flat_hash_map<NameHash, EventHandler> events;
  ska::flat_hash_map<EventId, std::vector<NameHash>> eventIdToHandlers;
  std::vector<DelayedEvent> delayedEvents;
  EntityContainer entityContainer;
  TemplatesMap templates;


  ecs::TypeId EntityIdTypeId;
  ecs::ComponentId eidComponentId;

  EcsManager()
  {
    EntityIdTypeId = ecs::type_registration<ecs::EntityId>(typeMap);
    eidComponentId = ecs::get_or_add_component(componentMap, EntityIdTypeId, "eid");
  }

  ecs::EntityId create_entity(ArchetypeId archetypeId, const InitializerList &template_init, InitializerList &&override_list)
  {
    auto it = archetypeMap.find(archetypeId);
    if (it == archetypeMap.end())
    {
      printf("Archetype not found\n");
      return EntityId();
    }
    Archetype &archetype = *it->second;
    uint32_t entityIndex = archetype.entityCount;
    ecs::EntityId eid = entityContainer.create_entity(archetypeId, entityIndex);
    override_list.push_back(ecs::ComponentInit(eidComponentId, ecs::EntityId(eid)));
    // can be not equal if template has unregistered components. Not terrible, but not good. In this case, we should skip them
    assert(archetype.type.size() <= template_init.size());
    archetype.add_entity(typeMap, template_init, std::move(override_list));
    return eid;
  }
  ecs::EntityId create_entity(TemplateId templateId, InitializerList &&init_list = {})
  {
    auto it = templates.find(templateId);
    if (it == templates.end())
    {
      printf("[ECS] Error: Template not found\n");
      return EntityId();
    }
    const Template &templateRecord = it->second;
    return create_entity(templateRecord.archetypeId, templateRecord.args, std::move(init_list));
  }

  std::vector<EntityId> create_entities(ArchetypeId archetypeId, const InitializerList &template_init, InitializerSoaList &&override_soa_list)
  {
    auto it = archetypeMap.find(archetypeId);
    if (it == archetypeMap.end())
    {
      printf("Archetype not found\n");
      return {};
    }
    Archetype &archetype = *it->second;
    int requiredEntityCount = override_soa_list.size();
    std::vector<EntityId> eids;
    eids.reserve(requiredEntityCount);
    uint32_t entityIndex = archetype.entityCount;
    for (int i = 0; i < requiredEntityCount; i++)
    {
      eids.push_back(entityContainer.create_entity(archetypeId, entityIndex + i));
    }

    // need to create copy to return list of eids
    override_soa_list.push_back(ecs::ComponentSoaInit(eidComponentId, std::vector<EntityId>(eids)));
    assert(archetype.type.size() == template_init.size());
    archetype.add_entities(typeMap, template_init, std::move(override_soa_list));
    return eids;
  }

  std::vector<EntityId> create_entities(TemplateId templateId, InitializerSoaList &&init_soa_list)
  {
    auto it = templates.find(templateId);
    if (it == templates.end())
    {
      printf("[ECS] Error: Template not found\n");
      return {};
    }
    const Template &templateRecord = it->second;
    return create_entities(templateRecord.archetypeId, templateRecord.args, std::move(init_soa_list));
  }

  bool destroy_entity(ecs::EntityId eid)
  {
    ecs::ArchetypeId archetypeId;
    uint32_t componentIndex;
    if (entityContainer.get(eid, archetypeId, componentIndex))
    {
      auto it = archetypeMap.find(archetypeId);
      if (it == archetypeMap.end())
      {
        printf("Archetype not found\n");
        return false;
      }
      Archetype &archetype = *it->second;
      archetype.remove_entity(typeMap, componentIndex);
      entityContainer.destroy_entity(eid);
      return true;
    }
    return false;
  }
};
TemplateId template_registration(
  EcsManager &manager,
  ComponentId eid_component_id,
  const char *_name,
  const std::span<TemplateId> &parent_templates,
  InitializerList &&components,
  ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands);

inline ComponentId get_or_add_component(EcsManager &manager, TypeId typeId, const char *component_name)
{
  return ecs::get_or_add_component(manager.componentMap, typeId, component_name);
}


inline TemplateId template_registration(EcsManager &manager, const char *_name, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands)
{
  return template_registration(manager, manager.eidComponentId, _name, {}, std::move(components), chunk_size_power);
}

inline TemplateId template_registration(EcsManager &manager, const char *_name, TemplateId parent_template, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands)
{
  return template_registration(manager, manager.eidComponentId, _name, {&parent_template, 1}, std::move(components), chunk_size_power);
}

inline void perform_system(const System &system)
{
  for (const auto &[archetypeId, archetypeRecord] : system.archetypesCache)
  {
    system.update_archetype(*archetypeRecord.archetype, archetypeRecord.toComponentIndex);
  }
}

inline void register_query(EcsManager &mgr, Query &&query)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    query.try_registrate(archetype.get());
  }
  printf("[ECS] Register query %s\n", query.uniqueName.c_str());
  mgr.queries[query.nameHash] = std::move(query);
}

inline void register_system(EcsManager &mgr, System &&system)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    system.try_registrate(archetype.get());
  }
  printf("[ECS] Register system %s\n", system.uniqueName.c_str());
  mgr.systems[system.nameHash] = std::move(system);
}


inline void register_event(EcsManager &mgr, EventHandler &&event)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    event.try_registrate(archetype.get());
  }
  printf("[ECS] Register system %s\n", event.uniqueName.c_str());
  for (EventId eventId : event.eventIds)
  {
    mgr.eventIdToHandlers[eventId].push_back(event.nameHash);
  }
  mgr.events[event.nameHash] = std::move(event);
}

inline void register_archetype(EcsManager &mgr, Archetype &&archetype)
{
  std::unique_ptr<Archetype> archetypePtr = std::make_unique<Archetype>(std::move(archetype));
  for (auto &[id, query] : mgr.queries)
  {
    query.try_registrate(archetypePtr.get());
  }
  for (auto &[id, query] : mgr.systems)
  {
    query.try_registrate(archetypePtr.get());
  }
  for (auto &[id, query] : mgr.events)
  {
    query.try_registrate(archetypePtr.get());
  }
  mgr.archetypeMap[archetype.archetypeId] = std::move(archetypePtr);
}

void perform_event_immediate(EcsManager &mgr, EventId event_id, const void *event_ptr);
void perform_event_immediate(EcsManager &mgr, EntityId eid, EventId event_id, const void *event_ptr);

void perform_delayed_events(EcsManager &mgr);

template <typename T>
inline void send_event_immediate(EcsManager &mgr, const T &event)
{
  perform_event_immediate(mgr, ecs::EventInfo<T>::eventId, &event);
}
template <typename T>
void send_event_immediate(EcsManager &mgr, ecs::EntityId eid, const T &event)
{
  perform_event_immediate(mgr, eid, ecs::EventInfo<T>::eventId, &event);
}

template <typename T>
inline void send_event(EcsManager &mgr, T &&event)
{
  static_assert(std::is_rvalue_reference<decltype(event)>::value);
  EcsManager::DelayedEvent delayedEvent;
  delayedEvent.eventData = ComponentData(std::move(event));
  delayedEvent.broadcastEvent = true;
  delayedEvent.eventId = ecs::EventInfo<T>::eventId;
  mgr.delayedEvents.push_back(std::move(delayedEvent));
}

template <typename T>
inline void send_event(EcsManager &mgr, ecs::EntityId eid, T &&event)
{
  static_assert(std::is_rvalue_reference<decltype(event)>::value);
  EcsManager::DelayedEvent delayedEvent;
  delayedEvent.eventData = ComponentData(std::move(event));
  delayedEvent.broadcastEvent = false;
  delayedEvent.eventId = ecs::EventInfo<T>::eventId;
  delayedEvent.entityId = eid;
  mgr.delayedEvents.push_back(std::move(delayedEvent));
}

} // namespace ecs
