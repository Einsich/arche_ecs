#pragma once

#include "ecs/config.h"
#include "ecs/type_declaration.h"
#include "ecs/component_declaration.h"
#include "ecs/entity_id.h"
#include "ecs/entity_container.h"
#include "ecs/archetype.h"
#include "ecs/template.h"
#include "ecs/query.h"
#include "ecs/event.h"

namespace ecs
{
using TemplatesMap = ska::flat_hash_map<TemplateId, Template>;
using ArchetypeMap = ska::flat_hash_map<ArchetypeId, std::unique_ptr<ecs_details::Archetype>>;

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
  ecs_details::EntityContainer entityContainer;
  TemplatesMap templates;


  ecs::TypeId EntityIdTypeId;
  ecs::ComponentId eidComponentId;

  EcsManager();

};


} // namespace ecs
