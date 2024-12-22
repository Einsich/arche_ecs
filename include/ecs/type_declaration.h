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

using DefaultConstructor = void (*)(void *mem);
using Destructor = void (*)(void *mem);
using CopyConstructor = void (*)(void *dest, const void *src);
using MoveConstructor = void (*)(void *dest, void *src);

struct TypeDeclaration
{
  std::string typeName;
  TypeId typeId;
  uint32_t sizeOfElement;
  uint32_t alignmentOfElement;
  bool isTriviallyRelocatable;

  DefaultConstructor construct_default;
  Destructor destruct;
  CopyConstructor copy_construct;
  MoveConstructor move_construct;
};

using TypeDeclarationMap = ska::flat_hash_map<TypeId, TypeDeclaration>;



inline const TypeDeclaration *find(const TypeDeclarationMap &type_map, TypeId type_id)
{
  const auto it = type_map.find(type_id);
  return it != type_map.end() ? &it->second : nullptr;
}

inline const TypeDeclaration *find(const TypeDeclarationMap &type_map, const char *type_name)
{
  const auto it = type_map.find(hash(type_name));
  return it != type_map.end() ? &it->second : nullptr;
}

template<typename T>
TypeDeclaration create_type_declaration()
{
  TypeDeclaration type_declaration;
  type_declaration.typeName = ecs::TypeInfo<T>::typeName;
  type_declaration.typeId = ecs::TypeInfo<T>::typeId;
  type_declaration.isTriviallyRelocatable = ecs::TypeInfo<T>::isTriviallyRelocatable;
  type_declaration.sizeOfElement = sizeof(T);
  type_declaration.alignmentOfElement = alignof(T);
  type_declaration.construct_default = construct_default<T>;
  type_declaration.destruct = destruct<T>;
  type_declaration.copy_construct = copy_construct<T>;
  type_declaration.move_construct = move_construct<T>;
  return type_declaration;
}

template<typename T>
TypeId type_registration(TypeDeclarationMap &type_map)
{
  TypeDeclaration type_declaration = create_type_declaration<T>();
  TypeId typeId = type_declaration.typeId;
  type_map[typeId] = std::move(type_declaration);
  return typeId;
}

template<typename T>
struct TypeInfo;

#define ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, IS_TRIVIALLY_RELOCATABLE) \
  template<> \
  struct ecs::TypeInfo<CPP_TYPE> \
  { \
    static constexpr ecs::TypeId typeId = ecs::hash(STRING_ALIAS); \
    static constexpr const char *typeName = STRING_ALIAS; \
    static constexpr bool isTriviallyRelocatable = IS_TRIVIALLY_RELOCATABLE; \
  };

template<typename T>
struct is_trivially_relocatable
{
  static constexpr bool value = sizeof(T) < 1024;
};

#define ECS_TYPE_DECLARATION(CPP_TYPE) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, #CPP_TYPE, is_trivially_relocatable<CPP_TYPE>::value)
#define ECS_RELOCATABLE_TYPE_DECLARATION(CPP_TYPE) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, #CPP_TYPE, true)
#define ECS_NON_RELOCATABLE_TYPE_DECLARATION(CPP_TYPE) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, #CPP_TYPE, false)

#define ECS_TYPE_DECLARATION_ALIAS(CPP_TYPE, STRING_ALIAS) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, is_trivially_relocatable<CPP_TYPE>::value)
#define ECS_RELOCATABLE_TYPE_DECLARATION_ALIAS(CPP_TYPE, STRING_ALIAS) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, true)
#define ECS_NON_RELOCATABLE_TYPE_DECLARATION_ALIAS(CPP_TYPE, STRING_ALIAS) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, false)

struct TypeDeclarationInfo final : public TypeDeclaration
{
  static inline TypeDeclarationInfo *tail = nullptr;
  TypeDeclarationInfo *next = nullptr;

  TypeDeclarationInfo(TypeDeclaration &&type_declaration) : TypeDeclaration(std::move(type_declaration))
  {
    next = tail;
    tail = this;
  }

  template<typename C>
  static void iterate_all(C &&callback /*(const TypeDeclaration &type_declaration)->void*/)
  {
    for (TypeDeclarationInfo *info = tail; info; info = info->next)
    {
      callback((const TypeDeclaration &)*info);
    }
  }

};
#define __CONCAT_HELPER__(x, y) x##y
#define __CONCAT__(x, y) __CONCAT_HELPER__(x, y)

#define ECS_TYPE_REGISTRATION(CPP_TYPE) \
  static ecs::TypeDeclarationInfo __CONCAT__(type_declaration_info_register, __COUNTER__)(ecs::create_type_declaration<CPP_TYPE>());

} // namespace ecs
