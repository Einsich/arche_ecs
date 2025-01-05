#include "ecs/ecs_manager.h"
#include "ecs/codegen_helpers.h"
#include "ecs/type_declaration_helper.h"
#include "ecs/builtin_events.h"

#include <span>
#include <assert.h>

ECS_TYPE_DECLARATION_ALIAS(ecs::EntityId, "EntityId")

namespace ecs_details
{
  ecs::ArchetypeId get_or_create_archetype(ecs::EcsManager &mgr, const ecs::InitializerList &components, ecs::ArchetypeChunkSize chunk_size_power, const char *template_name);
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

static ecs::EntityId create_entity(EcsManager &mgr, ArchetypeId archetypeId, const InitializerList &template_init, InitializerList &&override_list)
{
  auto it = mgr.archetypeMap.find(archetypeId);
  if (it == mgr.archetypeMap.end())
  {
    printf("Archetype not found\n");
    return EntityId();
  }
  ecs_details::Archetype &archetype = *it->second;
  uint32_t entityIndex = archetype.entityCount;
  ecs::EntityId eid = mgr.entityContainer.create_entity(archetypeId, entityIndex);
  override_list.push_back(ecs::ComponentInit(mgr.eidComponentId, ecs::EntityId(eid)));
  // can be not equal if template has unregistered components. Not terrible, but not good. In this case, we should skip them
  assert(archetype.type.size() <= template_init.size());
  ecs_details::add_entity_to_archetype(archetype, mgr.typeMap, template_init, std::move(override_list));

  const OnAppear event;
  perform_event_immediate(mgr, archetypeId, entityIndex, ecs::EventInfo<OnAppear>::eventId, &event);

  return eid;
}

ecs::EntityId create_entity(EcsManager &mgr, TemplateId templateId, InitializerList &&init_list)
{
  auto it = mgr.templates.find(templateId);
  if (it == mgr.templates.end())
  {
    printf("[ECS] Error: Template not found\n");
    return EntityId();
  }
  const Template &templateRecord = it->second;
  return create_entity(mgr, templateRecord.archetypeId, templateRecord.args, std::move(init_list));
}

static std::vector<EntityId> create_entities(EcsManager &mgr, ArchetypeId archetypeId, const InitializerList &template_init, InitializerSoaList &&override_soa_list)
{
  auto it = mgr.archetypeMap.find(archetypeId);
  if (it == mgr.archetypeMap.end())
  {
    printf("Archetype not found\n");
    return {};
  }
  ecs_details::Archetype &archetype = *it->second;
  uint32_t requiredEntityCount = override_soa_list.size();
  uint32_t entityIndex = archetype.entityCount;
  std::vector<EntityId> eids = mgr.entityContainer.create_entities(archetypeId, entityIndex, requiredEntityCount);

  // need to create copy to return list of eids
  override_soa_list.push_back(ecs::ComponentSoaInit(mgr.eidComponentId, std::vector<EntityId>(eids)));
  assert(archetype.type.size() == template_init.size());
  ecs_details::add_entities_to_archetype(archetype, mgr.typeMap, template_init, std::move(override_soa_list));

  const OnAppear event;
  for (uint32_t i = 0; i < requiredEntityCount; i++)
  {
    perform_event_immediate(mgr, archetypeId, entityIndex + i, ecs::EventInfo<OnAppear>::eventId, &event);
  }
  return eids;
}

std::vector<EntityId> create_entities(EcsManager &mgr, TemplateId templateId, InitializerSoaList &&init_soa_list)
{
  auto it = mgr.templates.find(templateId);
  if (it == mgr.templates.end())
  {
    printf("[ECS] Error: Template not found\n");
    return {};
  }
  const Template &templateRecord = it->second;
  return create_entities(mgr, templateRecord.archetypeId, templateRecord.args, std::move(init_soa_list));
}

bool destroy_entity(EcsManager &mgr, ecs::EntityId eid)
{
  ecs::ArchetypeId archetypeId;
  uint32_t componentIndex;
  if (mgr.entityContainer.get(eid, archetypeId, componentIndex))
  {
    auto it = mgr.archetypeMap.find(archetypeId);
    if (it == mgr.archetypeMap.end())
    {
      printf("Archetype not found\n");
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
      perform_event_immediate(mgr, event.eventId, event.eventData.data());
    }
    else
    {
      perform_event_immediate(mgr, event.entityId, event.eventId, event.eventData.data());
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
    printf("Template \"%s\" already exists\n", _name);
    return templateId;
  }
  components.push_back(ecs::ComponentInit{eid_component_id, ecs::EntityId()});
  for (TemplateId parent_template : parent_templates)
  {
    auto it = mgr.templates.find(parent_template);
    if (it == mgr.templates.end())
    {
      printf("Parent template not found\n");
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
        components.push_back({componentId, componentInit.copy()});
      }
    }
  }
  ArchetypeId archetypeId = ecs_details::get_or_create_archetype(mgr, components, chunk_size_power, _name);

  Template templateRecord{std::string(_name), std::move(components), archetypeId, {}};

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
    if (entity.isAlive)
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
      printf("Archetype not found\n");
      return nullptr;
    }
    ecs_details::Archetype &archetype = *it->second;
    int collumnIdx = archetype.getComponentCollumnIndex(componentId);
    if (collumnIdx != -1)
    {
      return archetype.collumns[collumnIdx].get_data(componentIndex);
    }
  }
  return nullptr;
}

void *get_rw_component(EcsManager &mgr, EntityId eid, ComponentId componentId)
{
  return const_cast<void *>(get_component(mgr, eid, componentId));
}

} // namespace ecs