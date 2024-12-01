#pragma once

#include "ecs/config.h"
#include "ecs/type_declaration.h"
#include "ecs/component_declaration.h"
#include "ecs/entity_id.h"
#include "ecs/archetype.h"
#include "ecs/template.h"
#include "ecs/query.h"

#include <assert.h>
#include <span>

namespace ecs
{
using TemplatesMap = ska::flat_hash_map<TemplateId, Template>;

struct EcsManager
{
  TypeDeclarationMap typeMap;
  ComponentDeclarationMap componentMap;
  ska::flat_hash_map<ArchetypeId, Archetype> archetypeMap;
  ska::flat_hash_map<uint32_t, Query> queries;
  EntityContainer entityContainer;
  TemplatesMap templates;

  ecs::TypeId EntityIdTypeId;
  ecs::ComponentId eidComponentId;

  EcsManager()
  {
    EntityIdTypeId = ecs::type_registration<ecs::EntityId>(typeMap, "EntityId");
    eidComponentId = ecs::component_registration(componentMap, EntityIdTypeId, "eid");
  }

  ecs::EntityId create_entity(ArchetypeId archetypeId, const InitializerList &template_init, InitializerList override_list)
  {
    auto it = archetypeMap.find(archetypeId);
    if (it == archetypeMap.end())
    {
      printf("Archetype not found\n");
      return EntityId();
    }
    Archetype &archetype = it->second;
    uint32_t entityIndex = archetype.entityCount;
    ecs::EntityId eid = entityContainer.create_entity(archetypeId, entityIndex);
    override_list.push_back(ecs::ComponentInit(eidComponentId, ecs::EntityId(eid)));
    assert(archetype.type.size() == template_init.size());
    archetype.add_entity(typeMap, template_init, std::move(override_list));
    return eid;
  }
  ecs::EntityId create_entity(TemplateId templateId, InitializerList init_list = {})
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

  std::vector<EntityId> create_entities(ArchetypeId archetypeId, const InitializerList &template_init, InitializerSoaList override_soa_list)
  {
    auto it = archetypeMap.find(archetypeId);
    if (it == archetypeMap.end())
    {
      printf("Archetype not found\n");
      return {};
    }
    Archetype &archetype = it->second;
    int requiredEntityCount = override_soa_list.size();
    std::vector<EntityId> eids(requiredEntityCount);
    uint32_t entityIndex = archetype.entityCount;
    for (int i = 0; i < requiredEntityCount; i++)
    {
      eids[i] = entityContainer.create_entity(archetypeId, entityIndex + i);
    }

    // need to create copy to return list of eids
    override_soa_list.push_back(ecs::ComponentSoaInit(eidComponentId, std::vector<EntityId>(eids)));
    assert(archetype.type.size() == template_init.size());
    archetype.add_entities(typeMap, template_init, std::move(override_soa_list));
    return eids;
  }

  std::vector<EntityId> create_entities(TemplateId templateId, InitializerSoaList init_soa_list)
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
      Archetype &archetype = it->second;
      archetype.remove_entity(typeMap, componentIndex);
      entityContainer.destroy_entity(eid);
      return true;
    }
    return false;
  }
};
TemplateId template_registration(
  const ComponentDeclarationMap &component_map,
  const TypeDeclarationMap &type_map,
  ArchetypeMap &archetype_map,
  TemplatesMap &templates,
  ComponentId eid_component_id,
  const char *_name,
  const std::span<TemplateId> &parent_templates,
  InitializerList &&components,
  ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands);


inline TemplateId template_registration(EcsManager &manager, const char *_name, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands)
{
  return template_registration(manager.componentMap, manager.typeMap, manager.archetypeMap, manager.templates, manager.eidComponentId, _name, {}, std::move(components), chunk_size_power);
}

inline TemplateId template_registration(EcsManager &manager, const char *_name, TemplateId parent_template, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands)
{
  return template_registration(manager.componentMap, manager.typeMap, manager.archetypeMap, manager.templates, manager.eidComponentId, _name, {&parent_template, 1}, std::move(components), chunk_size_power);
}

inline void update_query(ArchetypeMap &archetype_map, const Query &query)
{
  for (const auto &archetypeRecord : query.archetypesCache)
  {
    auto it = archetype_map.find(archetypeRecord.archetypeId);
    if (it == archetype_map.end())
    {
      continue;
    }
    query.update_archetype(it->second, archetypeRecord.toComponentIndex);
  }
}

} // namespace ecs
