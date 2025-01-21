#pragma once

#include "ecs/config.h"
#include "ecs/component_init.h"

namespace ecs
{

struct TemplateInitializer
{
  ecs::AnyComponent component;
  ecs_details::tiny_string info;
  bool needToTrack;
};

struct Template
{
  ecs_details::tiny_string name;
  InitializerList args = InitializerList(InitializerList::Empty{});
  ArchetypeId archetypeId;
  std::vector<TemplateId> composition;
};

}