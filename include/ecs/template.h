#pragma once

#include "ecs/config.h"
#include "ecs/component_init.h"

namespace ecs
{

struct Template
{
  std::string name;
  InitializerList args = InitializerList(InitializerList::Empty{});
  ArchetypeId archetypeId;
  std::vector<TemplateId> composition;
};

}