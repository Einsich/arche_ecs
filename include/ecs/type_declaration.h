#pragma once
#include "ecs/config.h"
#include "ecs/fnv_hash.h"

namespace ecs
{
template <typename T>
void construct_default(void *data)
{
  new (data) T();
}

template <typename T>
void destruct(void *data)
{
  ((T *)data)->~T();
}

template <typename T>
void copy_construct(void *dest, const void *src)
{
  new (dest) T(*(T *)src);
}

template <typename T>
void move_construct(void *dest, void *src)
{
  new (dest) T(std::move(*(T *)src));
}
struct TypeDeclaration
{
  std::string typeName;
  TypeId typeId;
  uint32_t sizeOfElement;
  uint32_t alignmentOfElement;

  void (*construct_default)(void *data);
  void (*destruct)(void *data);
  void (*copy_construct)(void *dest, const void *src);
  void (*move_construct)(void *dest, void *src);
};

using TypeDeclarationMap = ska::flat_hash_map<TypeId, std::unique_ptr<TypeDeclaration>>;

template<typename T>
TypeId type_registration(TypeDeclarationMap &type_map, const char *type_name)
{
  TypeId typeId = hash(type_name);
  std::unique_ptr<TypeDeclaration> type_declaration = std::make_unique<TypeDeclaration>();
  type_declaration->typeName = type_name;
  type_declaration->typeId = typeId;
  type_declaration->sizeOfElement = sizeof(T);
  type_declaration->alignmentOfElement = alignof(T);
  type_declaration->construct_default = construct_default<T>;
  type_declaration->destruct = destruct<T>;
  type_declaration->copy_construct = copy_construct<T>;
  type_declaration->move_construct = move_construct<T>;
  type_map[typeId] = std::move(type_declaration);

  return typeId;
}

inline const TypeDeclaration *find(const TypeDeclarationMap &type_map, TypeId type_id)
{
  const auto it = type_map.find(type_id);
  return it != type_map.end() ? it->second.get() : nullptr;
}

inline const TypeDeclaration *find(const TypeDeclarationMap &type_map, const char *type_name)
{
  const auto it = type_map.find(hash(type_name));
  return it != type_map.end() ? it->second.get() : nullptr;
}
}