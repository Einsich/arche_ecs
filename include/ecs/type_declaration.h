#pragma once
#include "ecs/config.h"
#include "ecs/fnv_hash.h"

namespace ecs
{

using DefaultConstructor = void (*)(void *mem);
using Destructor = void (*)(void *mem);
using CopyConstructor = void (*)(void *dest, const void *src);
using MoveConstructor = void (*)(void *dest, void *src);

struct TypeDeclaration
{
  std::string typeName;
  TypeId typeId = 0;
  uint32_t sizeOfElement = 0;
  uint32_t alignmentOfElement = 1;
  bool isTriviallyRelocatable = false;
  bool isSingleton = false;

  DefaultConstructor construct_default = nullptr;
  Destructor destruct = nullptr;
  CopyConstructor copy_construct = nullptr;
  MoveConstructor move_construct = nullptr;
};

using TypeDeclarationMap = ska::flat_hash_map<TypeId, TypeDeclaration>;

} // namespace ecs
