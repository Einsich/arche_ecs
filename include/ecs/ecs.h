#pragma once

#include "ecs_manager.h"
#include "ecs/type_declaration_helper.h"
#include "ecs/builtin_events.h"
#include "codegen_attributes.h"

namespace ecs
{

void register_all_codegen_files(ecs::EcsManager &mgr);

void register_all_type_declarations(ecs::EcsManager &mgr);

void sort_systems(EcsManager &mgr);
void init_singletons(EcsManager &mgr);

ecs::EntityId create_entity_sync(EcsManager &mgr, TemplateId templateId, InitializerList &&init_list = InitializerList(InitializerList::Empty{}));
ecs::EntityId create_entity(EcsManager &mgr, TemplateId templateId, InitializerList &&init_list = InitializerList(InitializerList::Empty{}));

std::vector<EntityId> create_entities_sync(EcsManager &mgr, TemplateId templateId, InitializerSoaList &&init_soa_list);
std::vector<EntityId> create_entities(EcsManager &mgr, TemplateId templateId, InitializerSoaList &&init_soa_list);

bool destroy_entity_sync(EcsManager &mgr, ecs::EntityId eid);
void destroy_entity(EcsManager &mgr, ecs::EntityId eid);

void destroy_entities(EcsManager &mgr);

void track_changes(ecs::EcsManager &mgr);

void perform_delayed_entities_creation(EcsManager &mgr);

TemplateId template_registration(EcsManager &manager, TemplateInit &&template_init);

TemplateId template_registration(EcsManager &manager, const char *_name, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands);

TemplateId template_registration(EcsManager &manager, const char *_name, TemplateId parent_template, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands);



void perform_event_immediate(EcsManager &mgr, EventId event_id, const void *event_ptr);
void perform_event_immediate(EcsManager &mgr, EntityId eid, EventId event_id, const void *event_ptr);

void perform_delayed_events(EcsManager &mgr);

template <typename T>
void send_event_immediate(EcsManager &mgr, const T &event)
{
  perform_event_immediate(mgr, ecs::EventInfo<T>::eventId, &event);
}

template <typename T>
void send_event_immediate(EcsManager &mgr, ecs::EntityId eid, const T &event)
{
  perform_event_immediate(mgr, eid, ecs::EventInfo<T>::eventId, &event);
}

template <typename T>
void send_event(EcsManager &mgr, T &&event)
{
  static_assert(std::is_rvalue_reference<decltype(event)>::value);
  EcsManager::DelayedEvent delayedEvent;
  delayedEvent.eventData = ecs::Any(std::move(event), ecs::EventInfo<T>::eventId, 0u);
  delayedEvent.broadcastEvent = true;
  mgr.delayedEvents.push_back(std::move(delayedEvent));
}

template <typename T>
void send_event(EcsManager &mgr, ecs::EntityId eid, T &&event)
{
  static_assert(std::is_rvalue_reference<decltype(event)>::value);
  EcsManager::DelayedEvent delayedEvent;
  delayedEvent.eventData = ecs::Any(std::move(event), ecs::EventInfo<T>::eventId, 0u);
  delayedEvent.broadcastEvent = false;
  delayedEvent.entityId = eid;
  mgr.delayedEvents.push_back(std::move(delayedEvent));
}

const void *get_component(EcsManager &mgr, EntityId eid, ComponentId componentId);
void *get_rw_component(EcsManager &mgr, EntityId eid, ComponentId componentId);

template <typename T>
const T *get_component(EcsManager &mgr, EntityId eid, const char *component_name)
{
  return static_cast<const T *>(get_component(mgr, eid, get_component_id(TypeInfo<T>::typeId, component_name)));
}

template <typename T>
T *get_rw_component(EcsManager &mgr, EntityId eid, const char *component_name)
{
  return static_cast<T *>(get_rw_component(mgr, eid, get_component_id(TypeInfo<T>::typeId, component_name)));
}

template <typename T>
bool set_component(EcsManager &mgr, EntityId eid, const char *component_name, T &&value)
{
  T *component = get_rw_component<T>(mgr, eid, component_name);
  if (component)
  {
    *component = std::move(value);
  }
  return component != nullptr;
}

template <typename T>
bool set_component(EcsManager &mgr, EntityId eid, const char *component_name, const T &value)
{
  T *component = get_rw_component<T>(mgr, eid, component_name);
  if (component)
  {
    *component = value;
  }
  return component != nullptr;
}

}

ECS_TYPE_DECLARATION_ALIAS(ecs::EntityId, "EntityId")
