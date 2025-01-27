#pragma once

#include "ecs/config.h"
#include "ecs/component_init.h"

namespace ecs
{
using TrackedComponentMap = ska::flat_hash_set<ecs::NameHash>;
struct Template
{
  ecs_details::tiny_string name;
  InitializerList args = InitializerList(InitializerList::Empty{});
  ArchetypeId archetypeId;
  TrackedComponentMap trackedComponents;
  std::vector<TemplateId> composition;
};

struct TemplateInit
{
  InitializerList args = InitializerList(InitializerList::Empty{});
  std::vector<ecs_details::tiny_string> trackedComponents;
  ecs_details::tiny_string name;
  ArchetypeChunkSize chunkSizePower = ArchetypeChunkSize::Thousands;
};

}