#pragma once
#include "ecs/config.h"
#include "ecs/fnv_hash.h"
#include "ecs/tiny_string.h"

namespace ecs
{

using DefaultConstructor = void (*)(void *mem);
using Destructor = void (*)(void *mem);
using CopyConstructor = void (*)(void *dest, const void *src);
using MoveConstructor = void (*)(void *dest, void *src);
using CompareAndAssign = bool (*)(const void *new_value, void *old_value); // return true if value changed

struct TypeDeclaration
{
  ecs_details::tiny_string typeName;
  DefaultConstructor construct_default = nullptr;
  Destructor destruct = nullptr;
  CopyConstructor copy_construct = nullptr;
  MoveConstructor move_construct = nullptr;
  CompareAndAssign compare_and_assign = nullptr; // return true if value changed
  TypeId typeId = 0;
  uint32_t sizeOfElement = 0;
  uint32_t alignmentOfElement = 1;
  bool isTriviallyRelocatable = false;
  bool isSingleton = false;
};

static_assert(sizeof(TypeDeclaration) == 64);

using TypeDeclarationMap = ska::flat_hash_map<TypeId, TypeDeclaration>;

} // namespace ecs
