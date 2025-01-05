#pragma once

#include "ecs_manager.h"
#include "ecs/type_declaration_helper.h"
#include "codegen_attributes.h"

namespace ecs
{
void register_all_codegen_files(ecs::EcsManager &mgr);
void register_all_type_declarations(ecs::EcsManager &mgr);

ecs::EntityId create_entity(EcsManager &mgr, TemplateId templateId, InitializerList &&init_list = {});

std::vector<EntityId> create_entities(EcsManager &mgr, TemplateId templateId, InitializerSoaList &&init_soa_list);

bool destroy_entity(EcsManager &mgr, ecs::EntityId eid);



TemplateId template_registration(EcsManager &manager, const char *_name, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands);

TemplateId template_registration(EcsManager &manager, const char *_name, TemplateId parent_template, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands);



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

}

ECS_TYPE_DECLARATION_ALIAS(ecs::EntityId, "EntityId")
