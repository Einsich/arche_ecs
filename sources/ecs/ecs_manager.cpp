#include "ecs/ecs_manager.h"

#include <span>

namespace ecs
{

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

void perform_event_immediate(EcsManager &mgr, EntityId eid, EventId event_id, const void *event_ptr)
{
  ArchetypeId archetypeId;
  uint32_t componentIdx;
  if (mgr.entityContainer.get(eid, archetypeId, componentIdx))
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
            ecs::Archetype &archetype = *archetypeRecord.archetype;
            const ecs::ToComponentMap &toComponentIndex = archetypeRecord.toComponentIndex;
            handler.unicastEvent(archetype, toComponentIndex, componentIdx, event_id, event_ptr);
          }
        }
      }
    }
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

TemplateId template_registration(
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
  ArchetypeId archetypeId = get_or_create_archetype(mgr, components, chunk_size_power, _name);

  Template templateRecord{std::string(_name), std::move(components), archetypeId, {}};

  mgr.templates[templateId] = std::move(templateRecord);

  return templateId;
}

} // namespace ecs