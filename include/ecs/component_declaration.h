#pragma once
#include "ecs/config.h"
#include "ecs/fnv_hash.h"

namespace ecs
{


struct ComponentDeclaration
{
  TypeId typeId;
  std::string name;
  ComponentId componentId; // hash(hash(type), hash(name))
};

inline constexpr ComponentId get_component_id(TypeId typeId, const char *component_name)
{
  uint32_t typeWithSpace = hash(" ", typeId);
  return hash(component_name, typeWithSpace);
}

inline constexpr ComponentId get_component_id(const char *type_name, const char *component_name)
{
  TypeId typeId = hash(type_name);
  uint32_t typeWithSpace = hash(" ", typeId);
  return hash(component_name, typeWithSpace);
}

using ComponentDeclarationMap = ska::flat_hash_map<ComponentId, std::unique_ptr<ComponentDeclaration>>;

struct EcsManager;

ComponentId get_or_add_component(EcsManager &mgr, TypeId typeId, const char *component_name);

}