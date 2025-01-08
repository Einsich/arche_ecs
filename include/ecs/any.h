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
    static void * CopyConstructor(const void *src_data)
    {
      return new T(*(T *)src_data);
    }
    template <typename T>
    static void CopyConstructorInplace(void *dst_data, const void *src_data)
    {
      new(dst_data) T(*(T *)src_data);
    }
    static constexpr int SBO_ALIGNMENT = 32;
    static constexpr int SBO_SIZE = 32;
    struct Stack
    {
      alignas(SBO_ALIGNMENT) char data[SBO_SIZE];
      void (*destructor)(void *data);
      void (*copy_constructor)(void *dst_data, const void *src_data);
    };
    struct Heap
    {
      void *data;
      void (*destructor)(void *data);
      void *(*copy_constructor)(const void *src_data);
    };
    union Sbo
    {
      Stack stack;
      Heap heap;
    } sbo;

    uint32_t typeId;

    enum class Type : uint8_t
    {
      None,
      Stack,
      Heap
    } type = Type::None;

    Any() = default;

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    Any(ValueType &&data, uint32_t type_id) : typeId(type_id)
    {
      if constexpr (sizeof(T) <= SBO_SIZE && alignof(T) <= SBO_ALIGNMENT)
      {
        new (sbo.stack.data) T(std::move(data));
        sbo.stack.destructor = Destructor<T>;
        sbo.stack.copy_constructor = CopyConstructorInplace<T>;
        type = Type::Stack;
      }
      else
      {
        sbo.heap.data = new T(std::move(data));
        sbo.heap.destructor = DeleteDestructor<T>;
        sbo.heap.copy_constructor = CopyConstructor<T>;
        type = Type::Heap;
      }
    }

    template<typename T>
    Any(const T &data, uint32_t type_id) : typeId(type_id)
    {
      if constexpr (sizeof(T) <= SBO_SIZE && alignof(T) <= SBO_ALIGNMENT)
      {
        new (sbo.stack.data) T(data);
        sbo.stack.destructor = Destructor<T>;
        sbo.stack.copy_constructor = CopyConstructorInplace<T>;
        type = Type::Stack;
      }
      else
      {
        sbo.heap.data = new T(data);
        sbo.heap.destructor = DeleteDestructor<T>;
        sbo.heap.copy_constructor = CopyConstructor<T>;
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
        sbo.stack.destructor(sbo.stack.data);
      }
      else if (type == Type::Heap)
      {
        sbo.heap.destructor(sbo.heap.data);
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
    }

    Any copy() const
    {
      Any copy;
      if (type == Type::Stack)
      {
        copy.sbo.stack.copy_constructor = sbo.stack.copy_constructor;
        copy.sbo.stack.destructor = sbo.stack.destructor;
        sbo.stack.copy_constructor(copy.sbo.stack.data, sbo.stack.data);
        copy.type = Type::Stack;
      }
      else if (type == Type::Heap)
      {
        copy.sbo.heap.copy_constructor = sbo.heap.copy_constructor;
        copy.sbo.heap.destructor = sbo.heap.destructor;
        copy.sbo.heap.data = sbo.heap.copy_constructor(sbo.heap.data);
        copy.type = Type::Heap;
      }
      copy.typeId = typeId;
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
} // namespace ecs