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

constexpr ComponentId get_component_id(TypeId typeId, const char *component_name)
{
  uint32_t typeWithSpace = hash(" ", typeId);
  return hash(component_name, typeWithSpace);
}

constexpr ComponentId get_component_id(const char *type_name, const char *component_name)
{
  TypeId typeId = hash(type_name);
  uint32_t typeWithSpace = hash(" ", typeId);
  return hash(component_name, typeWithSpace);
}

using ComponentDeclarationMap = ska::flat_hash_map<ComponentId, std::unique_ptr<ComponentDeclaration>>;

ComponentId component_registration(ComponentDeclarationMap &component_map, TypeId typeId, const char *component_name)
{
  ComponentId componentId = get_component_id(typeId, component_name);
  auto componentDeclaration = std::make_unique<ComponentDeclaration>();
  componentDeclaration->typeId = typeId;
  componentDeclaration->name = component_name;
  componentDeclaration->componentId = componentId;
  component_map[componentId] = std::move(componentDeclaration);
  return componentId;
}

const ComponentDeclaration *find(const ComponentDeclarationMap &component_map, ComponentId component_id)
{
  auto it = component_map.find(component_id);
  return it != component_map.end() ? it->second.get() : nullptr;
}
}