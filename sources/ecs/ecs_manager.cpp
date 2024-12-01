#include "ecs/ecs_manager.h"

#include <span>

namespace ecs
{

TemplateId template_registration(
  const ComponentDeclarationMap &component_map,
  const TypeDeclarationMap &type_map,
  ArchetypeMap &archetype_map,
  TemplatesMap &templates,
  ComponentId eid_component_id,
  const char *_name,
  const std::span<TemplateId> &parent_templates,
  InitializerList &&components,
  ArchetypeChunkSize chunk_size_power)
{
  TemplateId templateId = hash(_name);
  if (templates.find(templateId) != templates.end())
  {
    printf("Template \"%s\" already exists\n", _name);
    return templateId;
  }
  components.push_back(ecs::ComponentInit{eid_component_id, ecs::EntityId()});
  for (TemplateId parent_template : parent_templates)
  {
    auto it = templates.find(parent_template);
    if (it == templates.end())
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
  ArchetypeId archetypeId = get_or_create_archetype(component_map, type_map, archetype_map, components, chunk_size_power);

  Template templateRecord{std::string(_name), std::move(components), archetypeId, {}};

  templates[templateId] = std::move(templateRecord);

  return templateId;
}

} // namespace ecs