#pragma once

#include "ecs/config.h"
#include "ecs/archetype.h"

namespace ecs
{

using ToComponentMap = std::vector<int>;

struct ArchetypeRecord
{
  ArchetypeId archetypeId;
  ToComponentMap toComponentIndex;
};

struct Query
{

  enum class ComponentAccess
  {
    READ_COPY,
    READ_ONLY,
    READ_WRITE,
    READ_ONLY_OPTIONAL,
    READ_WRITE_OPTIONAL,
  };

  struct ComponentAccessInfo
  {
    ComponentId componentId;
    ComponentAccess access;
  };

  std::string uniqueName;
  NameHash nameHash;
  std::vector<ComponentAccessInfo> querySignature;

  std::vector<ComponentId> requireComponents; // components without reading access
  std::vector<ComponentId> excludeComponents;

  bool try_registrate(const Archetype &archetype)
  {
    for (ComponentId componentId : requireComponents)
    {
      if (archetype.get_component_index(componentId) == -1)
      {
        return false;
      }
    }

    for (ComponentId componentId : excludeComponents)
    {
      if (archetype.get_component_index(componentId) != -1)
      {
        return false;
      }
    }

    ToComponentMap toComponentIndex;
    toComponentIndex.reserve(querySignature.size());
    for (const ComponentAccessInfo &componentAccessInfo : querySignature)
    {
      int componentIndex = archetype.get_component_index(componentAccessInfo.componentId);
      if (componentIndex == -1 && !(componentAccessInfo.access == ComponentAccess::READ_ONLY_OPTIONAL || componentAccessInfo.access == ComponentAccess::READ_WRITE_OPTIONAL))
      {
        return false;
      }
      toComponentIndex.push_back(componentIndex);
    }

    archetypesCache.push_back({archetype.archetypeId, std::move(toComponentIndex)});

    return true;
  }

  std::vector<ArchetypeRecord> archetypesCache;
};

struct System final : public Query
{
  void (*update_archetype)(Archetype &archetype, const ToComponentMap &to_archetype_component);
};

}