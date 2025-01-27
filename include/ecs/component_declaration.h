#pragma once
#include "ecs/config.h"
#include "ecs/fnv_hash.h"
#include "ecs/tiny_string.h"

namespace ecs
{


struct ComponentDeclaration
{
  ecs_details::tiny_string name;
  ComponentId componentId; // hash(hash(type), hash(name))
  TypeId typeId;
};

inline constexpr ComponentId get_component_id(TypeId typeId, NameHash component_name)
{
  return (uint64_t(component_name) << uint64_t(32)) | typeId;
}
inline constexpr ComponentId get_component_id(TypeId typeId, const char *component_name)
{
  return get_component_id(typeId, hash(component_name));
}
inline constexpr ComponentId get_component_id(const char *type_name, const char *component_name)
{
  return get_component_id(hash(type_name), hash(component_name));
}

inline constexpr TypeId get_type_id(ComponentId component_id)
{
  return uint32_t(component_id);
}

inline constexpr NameHash get_component_name_hash(ComponentId component_id)
{
  return component_id >> uint64_t(32);
}


struct EcsManager;

ComponentId get_or_add_component(EcsManager &mgr, TypeId typeId, const char *component_name);

}