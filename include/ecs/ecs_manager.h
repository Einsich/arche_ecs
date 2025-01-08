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

struct EcsManager
{
  using ComponentDeclarationMap = ska::flat_hash_map<ComponentId, std::unique_ptr<ComponentDeclaration>>;
  using ArchetypeMap = ska::flat_hash_map<ArchetypeId, std::unique_ptr<ecs_details::Archetype>>;
  using TemplatesMap = ska::flat_hash_map<TemplateId, Template>;

  struct DelayedEvent
  {
    ecs::Any eventData;
    EntityId entityId;
    bool broadcastEvent;
  };

  TypeDeclarationMap typeMap;
  ComponentDeclarationMap componentMap;
  ArchetypeMap archetypeMap;
  ska::flat_hash_map<NameHash, Query> queries;
  std::vector<System> systems;
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
