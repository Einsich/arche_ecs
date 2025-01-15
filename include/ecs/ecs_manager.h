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
#include "ecs/singleton_component.h"
#include "ecs/logger.h"

namespace ecs
{

struct EcsManager
{
  using ComponentDeclarationMap = ska::flat_hash_map<ComponentId, std::unique_ptr<ComponentDeclaration>>;
  using ArchetypeMap = ska::flat_hash_map<ArchetypeId, std::unique_ptr<ecs_details::Archetype>>;
  using TemplatesMap = ska::flat_hash_map<TemplateId, Template>;
  using SingletonComponentsMap = ska::flat_hash_map<TypeId, SingletonComponent>;

  struct DelayedEvent
  {
    ecs::Any eventData;
    EntityId entityId;
    bool broadcastEvent;
  };

  struct DelayedEntity
  {
    TemplateId templateId;
    ecs::EntityId eid;
    InitializerList initList;
  };

  struct DelayedEntitySoa
  {
    TemplateId templateId;
    std::vector<ecs::EntityId> eids;
    InitializerSoaList initSoaList;
  };

  TypeDeclarationMap typeMap;
  ComponentDeclarationMap componentMap;
  ArchetypeMap archetypeMap;
  ska::flat_hash_map<NameHash, Query> queries;
  std::vector<System> systems;
  ska::flat_hash_map<NameHash, EventHandler> events;
  ska::flat_hash_map<EventId, std::vector<NameHash>> eventIdToHandlers;
  std::vector<DelayedEvent> delayedEvents;
  std::vector<DelayedEntity> delayedEntities;
  std::vector<DelayedEntitySoa> delayedEntitiesSoa;
  std::vector<ecs::EntityId> delayedEntitiesDestroy;
  ecs_details::EntityContainer entityContainer;
  TemplatesMap templates;
  SingletonComponentsMap singletons;


  ecs::TypeId EntityIdTypeId;
  ecs::ComponentId eidComponentId;

  ecs::LogLevel currentLogLevel = ecs::LogLevel::Verbose;
  std::unique_ptr<ecs::ILogger> logger;

  EcsManager();

};

template <typename T>
ComponentId get_or_add_component(EcsManager &mgr, const char *component_name)
{
  return get_or_add_component(mgr, TypeInfo<T>::typeId, component_name);
}

} // namespace ecs
