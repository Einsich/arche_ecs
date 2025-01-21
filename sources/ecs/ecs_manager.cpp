#include "ecs/ecs_manager.h"
#include "ecs/codegen_helpers.h"
#include "ecs/type_declaration_helper.h"
#include "ecs/builtin_events.h"

#include <span>
#include <assert.h>

ECS_TYPE_DECLARATION_ALIAS(ecs::EntityId, "EntityId")

namespace ecs_details
{
  ecs::ArchetypeId get_or_create_archetype(ecs::EcsManager &mgr, ecs::InitializerList &components, ecs::ArchetypeChunkSize chunk_size_power, const char *template_name);
}

namespace ecs
{

static void perform_event_immediate(EcsManager &mgr, ArchetypeId archetypeId, uint32_t componentIdx, EventId event_id, const void *event_ptr);

EcsManager::EcsManager()
{
  TypeDeclaration entityIdTypeDeclaration = create_type_declaration<ecs::EntityId>();
  EntityIdTypeId = entityIdTypeDeclaration.typeId;
  typeMap[EntityIdTypeId] = std::move(entityIdTypeDeclaration);
  eidComponentId = ecs::get_or_add_component(*this, EntityIdTypeId, "eid");
}

void register_all_codegen_files(ecs::EcsManager &mgr)
{
  for (const ecs_details::CodegenFileRegistration *info = ecs_details::CodegenFileRegistration::tail; info; info = info->next)
  {
    info->registrationFunction(mgr);
  }
}

void register_all_type_declarations(ecs::EcsManager &mgr)
{
  for (ecs_details::TypeDeclarationInfo *info = ecs_details::TypeDeclarationInfo::tail; info; info = info->next)
  {
    const ecs::TypeDeclaration &type_declaration = info->type_declaration;
    mgr.typeMap[type_declaration.typeId] = type_declaration;
  }
}

ComponentId get_or_add_component(EcsManager &mgr, TypeId typeId, const char *component_name)
{
  ComponentId componentId = get_component_id(typeId, component_name);
  auto it = mgr.componentMap.find(componentId);
  if (it != mgr.componentMap.end())
  {
    return componentId;
  }
  auto componentDeclaration = std::make_unique<ComponentDeclaration>();
  componentDeclaration->typeId = typeId;
  componentDeclaration->name = component_name;
  componentDeclaration->componentId = componentId;
  mgr.componentMap[componentId] = std::move(componentDeclaration);
  return componentId;
}

static void create_entity_sync(EcsManager &mgr, ecs::EntityId eid, ecs_details::Archetype &archetype, const InitializerList &template_init, InitializerList &&override_list)
{
  uint32_t entityIndex = archetype.entityCount;
  // entity was destroyed already
  if (!mgr.entityContainer.mutate(eid, archetype.archetypeId, entityIndex))
    return;
  override_list.push_back(ecs::ComponentInit(mgr.eidComponentId, ecs::EntityId(eid)));
  // can be not equal if template has unregistered components. Not terrible, but not good. In this case, we should skip them
  assert(archetype.type.size() <= template_init.size());
  ecs_details::add_entity_to_archetype(archetype, mgr, template_init, std::move(override_list));

  const OnAppear event;
  perform_event_immediate(mgr, archetype.archetypeId, entityIndex, ecs::EventInfo<OnAppear>::eventId, &event);
}


ecs::EntityId create_entity_sync(EcsManager &mgr, TemplateId templateId, InitializerList &&init_list)
{
  auto it = mgr.templates.find(templateId);
  if (it == mgr.templates.end())
  {
    ECS_LOG_ERROR(mgr).log("Template with hash %x not found", templateId);
    return EntityId();
  }
  const Template &templateRecord = it->second;

  auto it2 = mgr.archetypeMap.find(templateRecord.archetypeId);
  if (it2 == mgr.archetypeMap.end())
  {
    ECS_LOG_ERROR(mgr).log("Archetype with hash %x not found", templateRecord.archetypeId);
    return EntityId();
  }
  ecs::EntityId eid = mgr.entityContainer.allocate_entity(ecs_details::EntityState::Alive);
  create_entity_sync(mgr, eid, *it2->second, templateRecord.args, std::move(init_list));
  return eid;
}

ecs::EntityId create_entity(EcsManager &mgr, TemplateId templateId, InitializerList &&init_list)
{
  ecs::EntityId eid = mgr.entityContainer.allocate_entity(ecs_details::EntityState::AsyncCreation);
  mgr.delayedEntities.push_back(ecs::EcsManager::DelayedEntity(templateId, eid, std::move(init_list)));
  return eid;
}

static std::vector<EntityId> create_entities(EcsManager &mgr, std::vector<EntityId> &&eids, ecs_details::Archetype &archetype, const InitializerList &template_init, InitializerSoaList &&override_soa_list)
{
  uint32_t requiredEntityCount = override_soa_list.size();
  const uint32_t startEntityIndex = archetype.entityCount;
  uint32_t entityIndex = startEntityIndex;

  eids.erase(eids.begin(), std::remove_if(eids.begin(), eids.end(), [&mgr, &archetype, &entityIndex](EntityId eid) {
    return !mgr.entityContainer.mutate(eid, archetype.archetypeId, entityIndex++);
  }));

  override_soa_list.push_back(ecs::ComponentSoaInit(mgr.eidComponentId, std::move(eids)));
  assert(archetype.type.size() == template_init.size());
  ecs_details::add_entities_to_archetype(archetype, mgr, template_init, std::move(override_soa_list));

  const OnAppear event;
  for (uint32_t i = 0; i < requiredEntityCount; i++)
  {
    perform_event_immediate(mgr, archetype.archetypeId, startEntityIndex + i, ecs::EventInfo<OnAppear>::eventId, &event);
  }
  return eids;
}

std::vector<EntityId> create_entities_sync(EcsManager &mgr, TemplateId templateId, InitializerSoaList &&init_soa_list)
{
  auto it = mgr.templates.find(templateId);
  if (it == mgr.templates.end())
  {
    ECS_LOG_ERROR(mgr).log("Template with hash %x not found", templateId);
    return {};
  }
  const Template &templateRecord = it->second;

  auto it2 = mgr.archetypeMap.find(templateRecord.archetypeId);
  if (it2 == mgr.archetypeMap.end())
  {
    ECS_LOG_ERROR(mgr).log("Archetype with hash %x not found", templateRecord.archetypeId);
    return {};
  }
  ecs_details::Archetype &archetype = *it2->second;
  uint32_t requiredEntityCount = init_soa_list.size();
  std::vector<EntityId> eids = mgr.entityContainer.allocate_entities(requiredEntityCount, ecs_details::EntityState::Alive);
  create_entities(mgr, std::vector<EntityId>(eids), archetype, templateRecord.args, std::move(init_soa_list));
  return eids;
}

std::vector<EntityId> create_entities(EcsManager &mgr, TemplateId templateId, InitializerSoaList &&init_soa_list)
{
  uint32_t requiredEntityCount = init_soa_list.size();
  std::vector<EntityId> eids = mgr.entityContainer.allocate_entities(requiredEntityCount, ecs_details::EntityState::AsyncCreation);
  mgr.delayedEntitiesSoa.push_back({templateId, std::vector<EntityId>(eids), std::move(init_soa_list)});
  return eids;
}

bool destroy_entity_sync(EcsManager &mgr, ecs::EntityId eid)
{
  ecs::ArchetypeId archetypeId;
  uint32_t componentIndex;
  if (mgr.entityContainer.get(eid, archetypeId, componentIndex))
  {
    auto it = mgr.archetypeMap.find(archetypeId);
    if (it == mgr.archetypeMap.end())
    {
      ECS_LOG_ERROR(mgr).log("Archetype with hash %x not found", archetypeId);
      return false;
    }
    const OnDisappear event;
    perform_event_immediate(mgr, archetypeId, componentIndex, ecs::EventInfo<OnDisappear>::eventId, &event);

    ecs_details::Archetype &archetype = *it->second;
    ecs_details::remove_entity_from_archetype(archetype, mgr.typeMap, componentIndex);
    mgr.entityContainer.destroy_entity(eid);
    return true;
  }
  return false;
}

void destroy_entity(EcsManager &mgr, ecs::EntityId eid)
{
  if (mgr.entityContainer.mark_as_destroyed(eid))
    mgr.delayedEntitiesDestroy.push_back(eid);
}

void perform_delayed_entities_creation(EcsManager &mgr)
{
  // need take into account that entity can be added/removed during OnAppear/OnDisappear events

  uint32_t delayedEntityDestroyCount = mgr.delayedEntitiesDestroy.size();
  uint32_t delayedEntityCount = mgr.delayedEntities.size();
  uint32_t delayedEntitySoaCount = mgr.delayedEntitiesSoa.size();

  for (uint32_t i = 0, n = delayedEntityDestroyCount; i < n; i++)
  {
    destroy_entity_sync(mgr, mgr.delayedEntitiesDestroy[i]);
  }

  for (uint32_t i = 0, n = delayedEntityCount; i < n; i++)
  {
    EcsManager::DelayedEntity &entity = mgr.delayedEntities[i];
    auto it = mgr.templates.find(entity.templateId);
    if (it == mgr.templates.end())
    {
      ECS_LOG_ERROR(mgr).log("Template with hash %x not found", entity.templateId);
      continue;
    }
    const Template &templateRecord = it->second;

    auto it2 = mgr.archetypeMap.find(templateRecord.archetypeId);
    if (it2 == mgr.archetypeMap.end())
    {
      ECS_LOG_ERROR(mgr).log("Archetype with hash %x not found", templateRecord.archetypeId);
      continue;
    }
    create_entity_sync(mgr, entity.eid, *it2->second, templateRecord.args, std::move(entity.initList));
  }

  for (uint32_t i = 0, n = delayedEntitySoaCount; i < n; i++)
  {
    EcsManager::DelayedEntitySoa &entity = mgr.delayedEntitiesSoa[i];
    auto it = mgr.templates.find(entity.templateId);
    if (it == mgr.templates.end())
    {
      ECS_LOG_ERROR(mgr).log("Template with hash %x not found", entity.templateId);
      continue;
    }
    const Template &templateRecord = it->second;

    auto it2 = mgr.archetypeMap.find(templateRecord.archetypeId);
    if (it2 == mgr.archetypeMap.end())
    {
      ECS_LOG_ERROR(mgr).log("Archetype with hash %x not found", templateRecord.archetypeId);
      continue;
    }
    create_entities(mgr, std::move(entity.eids), *it2->second, templateRecord.args, std::move(entity.initSoaList));
  }
  mgr.delayedEntities.erase(mgr.delayedEntities.begin(), mgr.delayedEntities.begin() + delayedEntityCount);
  mgr.delayedEntitiesSoa.erase(mgr.delayedEntitiesSoa.begin(), mgr.delayedEntitiesSoa.begin() + delayedEntitySoaCount);
  mgr.delayedEntitiesDestroy.erase(mgr.delayedEntitiesDestroy.begin(), mgr.delayedEntitiesDestroy.begin() + delayedEntityDestroyCount);
}

void perform_event_immediate(EcsManager &mgr, EventId event_id, const void *event_ptr)
{
  auto it = mgr.eventIdToHandlers.find(event_id);
  if (it != mgr.eventIdToHandlers.end())
  {
    for (NameHash queryId : it->second)
    {
      auto hndlIt = mgr.events.find(queryId);
      if (hndlIt != mgr.events.end())
      {
        EventHandler &handler = hndlIt->second;
        for (const auto &[archetypeId, archetypeRecord] : handler.archetypesCache)
        {
          handler.broadcastEvent(*archetypeRecord.archetype, archetypeRecord.toComponentIndex, event_id, event_ptr);
        }
      }
    }
  }
}

static void perform_event_immediate(EcsManager &mgr, ArchetypeId archetypeId, uint32_t componentIdx, EventId event_id, const void *event_ptr)
{
  auto it = mgr.eventIdToHandlers.find(event_id);
  if (it != mgr.eventIdToHandlers.end())
  {
    for (NameHash queryId : it->second)
    {
      auto hndlIt = mgr.events.find(queryId);
      if (hndlIt != mgr.events.end())
      {
        EventHandler &handler = hndlIt->second;
        auto ait = handler.archetypesCache.find(archetypeId);
        if (ait != handler.archetypesCache.end())
        {
          const auto &archetypeRecord = ait->second;
          ecs_details::Archetype &archetype = *archetypeRecord.archetype;
          const ecs::ToComponentMap &toComponentIndex = archetypeRecord.toComponentIndex;
          handler.unicastEvent(archetype, toComponentIndex, componentIdx, event_id, event_ptr);
        }
      }
    }
  }
}

void perform_event_immediate(EcsManager &mgr, EntityId eid, EventId event_id, const void *event_ptr)
{
  ArchetypeId archetypeId;
  uint32_t componentIdx;
  if (mgr.entityContainer.get(eid, archetypeId, componentIdx))
  {
    perform_event_immediate(mgr, archetypeId, componentIdx, event_id, event_ptr);
  }
}

void perform_delayed_events(EcsManager &mgr)
{
  for (const EcsManager::DelayedEvent &event : mgr.delayedEvents)
  {
    if (event.broadcastEvent)
    {
      perform_event_immediate(mgr, event.eventData.typeId, event.eventData.data());
    }
    else
    {
      perform_event_immediate(mgr, event.entityId, event.eventData.typeId, event.eventData.data());
    }
  }
  mgr.delayedEvents.clear();
}

static TemplateId template_registration(
  EcsManager &mgr,
  ComponentId eid_component_id,
  const char *_name,
  const std::span<TemplateId> &parent_templates,
  InitializerList &&components,
  ArchetypeChunkSize chunk_size_power)
{
  TemplateId templateId = hash(_name);
  if (mgr.templates.find(templateId) != mgr.templates.end())
  {
    ECS_LOG_ERROR(mgr).log("Template \"%s\" already exists", _name);
    return templateId;
  }
  components.push_back(ecs::ComponentInit{eid_component_id, ecs::EntityId()});
  for (TemplateId parent_template : parent_templates)
  {
    auto it = mgr.templates.find(parent_template);
    if (it == mgr.templates.end())
    {
      ECS_LOG_ERROR(mgr).log("Parent template with hash %x not found", parent_template);
      return templateId;
    }
    const Template &parentTemplate = it->second;
    for (const auto &[componentId, componentInit] : parentTemplate.args.args)
    {
      if (components.args.count(componentId) > 0)
      {
        continue;
      }
      else
      {
        components.push_back(componentInit.copy());
      }
    }
  }
  ArchetypeId archetypeId = ecs_details::get_or_create_archetype(mgr, components, chunk_size_power, _name);

  Template templateRecord{ecs_details::tiny_string(_name), std::move(components), archetypeId, {}};

  mgr.templates[templateId] = std::move(templateRecord);

  return templateId;
}

TemplateId template_registration(EcsManager &manager, const char *_name, InitializerList &&components, ArchetypeChunkSize chunk_size_power)
{
  return template_registration(manager, manager.eidComponentId, _name, {}, std::move(components), chunk_size_power);
}

TemplateId template_registration(EcsManager &manager, const char *_name, TemplateId parent_template, InitializerList &&components, ArchetypeChunkSize chunk_size_power)
{
  return template_registration(manager, manager.eidComponentId, _name, {&parent_template, 1}, std::move(components), chunk_size_power);
}

void destroy_entities(EcsManager &mgr)
{
  const OnDisappear event;
  for (const ecs_details::EntityRecord &entity : mgr.entityContainer.entityRecords)
  {
    if (entity.entityState == ecs_details::EntityState::Alive)
    {
      perform_event_immediate(mgr, entity.archetypeId, entity.componentIndex, ecs::EventInfo<OnDisappear>::eventId, &event);
    }
  }
  for (auto &[id, archetype] : mgr.archetypeMap)
  {
    ecs_details::destroy_all_entities_from_archetype(*archetype, mgr.typeMap);
  }
  mgr.entityContainer.entityRecords.clear();
  mgr.entityContainer.freeIndices.clear();
}

const void *get_component(EcsManager &mgr, EntityId eid, ComponentId componentId)
{
  ecs::ArchetypeId archetypeId;
  uint32_t componentIndex;
  if (mgr.entityContainer.get(eid, archetypeId, componentIndex))
  {
    auto it = mgr.archetypeMap.find(archetypeId);
    if (it == mgr.archetypeMap.end())
    {
      ECS_LOG_ERROR(mgr).log("Archetype with hash %x not found", archetypeId);
      return nullptr;
    }
    ecs_details::Archetype &archetype = *it->second;
    int collumnIdx = archetype.getComponentCollumnIndex(componentId);
    if (collumnIdx != -1)
    {
      return archetype.getData(archetype.collumns[collumnIdx], componentIndex);
    }
  }
  return nullptr;
}

void *get_rw_component(EcsManager &mgr, EntityId eid, ComponentId componentId)
{
  return const_cast<void *>(get_component(mgr, eid, componentId));
}

void init_singletons(EcsManager &mgr)
{
  for (const auto &[typeId, typeDecl] : mgr.typeMap)
  {
    if (typeDecl.isSingleton)
    {
      if (typeDecl.construct_default)
      {
        mgr.singletons[typeId] = ecs::SingletonComponent(typeDecl);
      }
      else
      {
        ECS_LOG_ERROR(mgr).log("Singleton component %s has no default constructor", typeDecl.typeName.c_str());
      }
    }
  }
}

} // namespace ecs

namespace ecs_details
{

void consume_init_list(ecs::EcsManager &mgr, ecs::InitializerList &&init_list)
{
  init_list.args.clear();
  mgr.initializersPool.push_back(std::move(init_list.args));
}

ecs::InitializerList::type get_init_list(ecs::EcsManager &mgr)
{
  if (mgr.initializersPool.empty())
  {
    return {};
  }
  ecs::InitializerList::type initList = std::move(mgr.initializersPool.back());
  mgr.initializersPool.pop_back();
  return initList;
}
} // namespace ecs_details