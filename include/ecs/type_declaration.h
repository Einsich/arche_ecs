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

} // namespace ecs
