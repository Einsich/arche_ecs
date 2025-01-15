#pragma once

#include "ecs/config.h"

namespace ecs
{
  struct SingletonComponent
  {
    uint32_t sizeOf, alignmentOf;
    ecs::TypeId typeId;
    Destructor destructor = nullptr;
    void *data = nullptr;
    SingletonComponent() = default;
    SingletonComponent(const ecs::TypeDeclaration &decl) : sizeOf(decl.sizeOfElement), alignmentOf(decl.alignmentOfElement), typeId(decl.typeId)
    {
      typeId = decl.typeId;
      data = new (std::align_val_t(alignmentOf)) char(sizeOf);
      if (decl.construct_default)
        decl.construct_default(data);
      destructor = decl.destruct;
    }
    ~SingletonComponent()
    {
      if (data)
      {
        if (destructor)
          destructor(data);
        operator delete (data, sizeOf, std::align_val_t(alignmentOf));
      }
    }
    SingletonComponent(const SingletonComponent &) = delete;
    SingletonComponent(SingletonComponent &&other) noexcept
    {
      sizeOf = other.sizeOf;
      alignmentOf = other.alignmentOf;
      typeId = other.typeId;
      data = other.data;
      other.data = nullptr;
    }
    SingletonComponent &operator=(const SingletonComponent &other) = delete;
    SingletonComponent &operator=(SingletonComponent &&other) noexcept
    {
      if (this != &other)
      {
        typeId = other.typeId;
        data = other.data;
        other.data = nullptr;
      }
      return *this;
    }
  };

} // namespace ecs