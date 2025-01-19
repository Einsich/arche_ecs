#pragma once
#include "ecs/config.h"

namespace ecs
{
  struct Any
  {

    template <typename T>
    static void Destructor(void *data)
    {
      ((T *)data)->~T();
    }
    template <typename T>
    static void DeleteDestructor(void *data)
    {
      delete ((T *)data);
    }
    template <typename T>
    static void * CopyConstructor(void *, const void *src_data)
    {
      return new T(*(T *)src_data);
    }
    template <typename T>
    static void * CopyConstructorInplace(void *dst_data, const void *src_data)
    {
      return new(dst_data) T(*(T *)src_data);
    }
    static constexpr int SBO_ALIGNMENT = 32;
    static constexpr int SBO_SIZE = 32;
    struct Stack
    {
      alignas(SBO_ALIGNMENT) char data[SBO_SIZE];
    };
    struct Heap
    {
      void *data;
    };
    union Sbo
    {
      Stack stack;
      Heap heap;
    } sbo;

    static_assert(sizeof(sbo) == 32);

    void (*destructor)(void *data);
    void *(*copy_constructor)(void *dst_data, const void *src_data);
    ComponentId componentId;
    TypeId typeId;

    enum class Type : uint8_t
    {
      None,
      Stack,
      Heap
    } type = Type::None;

    Any() = default;

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    Any(ValueType &&data, TypeId type_id, ComponentId component_id) : componentId(component_id), typeId(type_id)
    {
      if constexpr (sizeof(T) <= SBO_SIZE && alignof(T) <= SBO_ALIGNMENT)
      {
        new (sbo.stack.data) T(std::move(data));
        destructor = Destructor<T>;
        copy_constructor = CopyConstructorInplace<T>;
        type = Type::Stack;
      }
      else
      {
        sbo.heap.data = new T(std::move(data));
        destructor = DeleteDestructor<T>;
        copy_constructor = CopyConstructor<T>;
        type = Type::Heap;
      }
    }

    template<typename T>
    Any(const T &data, TypeId type_id, ComponentId component_id) : componentId(component_id), typeId(type_id)
    {
      if constexpr (sizeof(T) <= SBO_SIZE && alignof(T) <= SBO_ALIGNMENT)
      {
        new (sbo.stack.data) T(data);
        sbo.destructor = Destructor<T>;
        sbo.copy_constructor = CopyConstructorInplace<T>;
        type = Type::Stack;
      }
      else
      {
        sbo.heap.data = new T(data);
        sbo.destructor = DeleteDestructor<T>;
        sbo.copy_constructor = CopyConstructor<T>;
        type = Type::Heap;
      }
    }

    void orphan()
    {
      type = Type::None;
    }

    ~Any()
    {
      if (type == Type::Stack)
      {
        destructor(sbo.stack.data);
      }
      else if (type == Type::Heap)
      {
        destructor(sbo.heap.data);
      }
    }

    Any(Any &&other)
    {
      if (other.type == Type::Stack)
      {
        sbo.stack = other.sbo.stack;
        type = Type::Stack;
      }
      else if (other.type == Type::Heap)
      {
        sbo.heap = other.sbo.heap;
        type = Type::Heap;
      }
      other.type = Type::None;
      typeId = other.typeId;
      componentId = other.componentId;
      destructor = other.destructor;
      copy_constructor = other.copy_constructor;
    }

    Any copy() const
    {
      Any copy;
      if (type == Type::Stack)
      {
        copy_constructor(copy.sbo.stack.data, sbo.stack.data);
        copy.type = Type::Stack;
      }
      else if (type == Type::Heap)
      {
        copy.sbo.heap.data = copy_constructor(nullptr, sbo.heap.data);
        copy.type = Type::Heap;
      }
      copy.typeId = typeId;
      copy.componentId = componentId;
      copy.destructor = destructor;
      copy.copy_constructor = copy_constructor;
      return copy;
    }

    Any &operator=(Any &&other)
    {
      if (this != &other)
      {
        this->~Any();
        new (this) Any(std::move(other));
      }
      return *this;
    }

    Any(const Any &other) = delete;

    Any &operator=(const Any &other) = delete;

    void *data()
    {
      if (type == Type::Stack)
      {
        return sbo.stack.data;
      }
      else if (type == Type::Heap)
      {
        return sbo.heap.data;
      }
      return nullptr;
    }
    const void *data() const
    {
      if (type == Type::Stack)
      {
        return sbo.stack.data;
      }
      else if (type == Type::Heap)
      {
        return sbo.heap.data;
      }
      return nullptr;
    }
  };
  static_assert(sizeof(Any) == 64);
  using AnyComponent = Any;
} // namespace ecs