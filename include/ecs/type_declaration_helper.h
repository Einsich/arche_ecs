#pragma once
#include "ecs/type_declaration.h"

namespace ecs_details
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

// return true if value changed
template <typename T>
bool compare_and_assign(const void *new_value, void *old_value)
{
  const T *new_ = (const T *)new_value;
  T *old = (T *)old_value;
  if (*old == *new_)
    return false;
  *old = *new_;
  return true;
}
// Helper trait to check if T supports operator==
template <typename T, typename = void>
struct is_equality_comparable : std::false_type {};

template <typename T>
struct is_equality_comparable<T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>> : std::true_type {};

// Convenience variable template
template <typename T>
constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;

} // namespace ecs_details

namespace ecs
{

template<typename T>
TypeDeclaration create_type_declaration()
{
  TypeDeclaration type_declaration;
  type_declaration.typeName = ecs::TypeInfo<T>::typeName;
  type_declaration.typeId = ecs::TypeInfo<T>::typeId;
  type_declaration.isTriviallyRelocatable = ecs::TypeInfo<T>::isTriviallyRelocatable;
  type_declaration.isSingleton = ecs::TypeInfo<T>::isSingleton;
  type_declaration.sizeOfElement = sizeof(T);
  type_declaration.alignmentOfElement = alignof(T);
  type_declaration.construct_default = ecs_details::construct_default<T>;
  type_declaration.destruct = ecs_details::destruct<T>;
  if constexpr (std::is_copy_constructible_v<T>)
    type_declaration.copy_construct = ecs_details::copy_construct<T>;
  if constexpr (std::is_move_constructible_v<T>)
    type_declaration.move_construct = ecs_details::move_construct<T>;
  if constexpr (std::is_copy_constructible_v<T> && ecs_details::is_equality_comparable_v<T>)
    type_declaration.compare_and_assign = ecs_details::compare_and_assign<T>;
  return type_declaration;
}

template<typename T>
struct TypeInfo;

#define ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, IS_TRIVIALLY_RELOCATABLE, IS_SINGLETON) \
  template<> \
  struct ecs::TypeInfo<CPP_TYPE> \
  { \
    static constexpr ecs::TypeId typeId = ecs::hash(STRING_ALIAS); \
    static constexpr const char *typeName = STRING_ALIAS; \
    static constexpr bool isTriviallyRelocatable = IS_TRIVIALLY_RELOCATABLE; \
    static constexpr bool isSingleton = IS_SINGLETON; \
  };

template<typename T>
struct is_trivially_relocatable
{
  static constexpr bool value = sizeof(T) < 1024;
};

#define ECS_TYPE_DECLARATION(CPP_TYPE) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, #CPP_TYPE, is_trivially_relocatable<CPP_TYPE>::value, false)
#define ECS_RELOCATABLE_TYPE_DECLARATION(CPP_TYPE) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, #CPP_TYPE, true, false)
#define ECS_NON_RELOCATABLE_TYPE_DECLARATION(CPP_TYPE) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, #CPP_TYPE, false, false)

#define ECS_TYPE_DECLARATION_ALIAS(CPP_TYPE, STRING_ALIAS) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, is_trivially_relocatable<CPP_TYPE>::value, false)
#define ECS_RELOCATABLE_TYPE_DECLARATION_ALIAS(CPP_TYPE, STRING_ALIAS) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, true, false)
#define ECS_NON_RELOCATABLE_TYPE_DECLARATION_ALIAS(CPP_TYPE, STRING_ALIAS) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, false, false)

#define ECS_SINGLETON_TYPE_DECLARATION(CPP_TYPE) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, #CPP_TYPE, false, true)
#define ECS_SINGLETON_TYPE_DECLARATION_ALIAS(CPP_TYPE, STRING_ALIAS) ECS_TYPE_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, false, true)

} // namespace ecs

namespace ecs_details
{

struct TypeDeclarationInfo
{
  ecs::TypeDeclaration type_declaration;
  static inline TypeDeclarationInfo *tail = nullptr;
  TypeDeclarationInfo *next = nullptr;

  TypeDeclarationInfo(ecs::TypeDeclaration &&_type_declaration) : type_declaration(std::move(_type_declaration))
  {
    next = tail;
    tail = this;
  }
};

#define __CONCAT_HELPER__(x, y) x##y
#define __CONCAT__(x, y) __CONCAT_HELPER__(x, y)

#define ECS_TYPE_REGISTRATION(CPP_TYPE) \
  static ecs_details::TypeDeclarationInfo __CONCAT__(type_declaration_info_register, __COUNTER__)(ecs::create_type_declaration<CPP_TYPE>());

} // namespace ecs_details