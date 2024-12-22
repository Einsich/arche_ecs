#pragma once
#include "ecs/config.h"

namespace ecs
{

  struct EcsManager;
  ComponentId get_or_add_component(EcsManager &manager, TypeId typeId, const char *component_name);

  struct ComponentData
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

    enum class Type : uint8_t
    {
      None,
      Stack,
      Heap
    } type = Type::None;

    ComponentData() = default;

    template<typename T>
    ComponentData(T &&data)
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

    void orphan()
    {
      type = Type::None;
    }

    ~ComponentData()
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

    ComponentData(ComponentData &&other)
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
    }

    ComponentData copy() const
    {
      ComponentData copy;
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
      return copy;
    }

    ComponentData &operator=(ComponentData &&other)
    {
      if (this != &other)
      {
        this->~ComponentData();
        new (this) ComponentData(std::move(other));
      }
      return *this;
    }

    ComponentData(const ComponentData &other) = delete;

    ComponentData &operator=(const ComponentData &other) = delete;

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

  struct ComponentDataSoa
  {
    void *vector_ptr;
    void *vector_data_ptr;
    uint32_t m_size;
    uint32_t m_sizeof;

    void (*destructor)(void *data);

    uint32_t size() const
    {
      return m_size;
    }

    void *get_data(uint32_t index)
    {
      return (char *)vector_data_ptr + index * m_sizeof;
    }

    template <typename T>
    static void Destructor(void *data)
    {
      delete (T *)data;
    }
    template<typename T>
    ComponentDataSoa(std::vector<T> &&_data) : destructor(Destructor<std::vector<T>>)
    {
      m_sizeof = sizeof(T);
      auto vec = new std::vector<T>(std::move(_data));
      m_size = vec->size();
      vector_ptr = vec;
      vector_data_ptr = vec->data();
    }
    ComponentDataSoa(ComponentDataSoa &&other) :
      vector_ptr(other.vector_ptr),
      vector_data_ptr(other.vector_data_ptr),
      m_size(other.m_size),
      m_sizeof(other.m_sizeof),
      destructor(other.destructor)
    {
      other.vector_ptr = nullptr;
      other.vector_data_ptr = nullptr;
    }

    ComponentDataSoa &operator=(ComponentDataSoa &&other)
    {
      if (this != &other)
      {
        this->~ComponentDataSoa();
        new (this) ComponentDataSoa(std::move(other));
      }
      return *this;
    }
    ~ComponentDataSoa()
    {
      if (vector_ptr)
      {
        destructor(vector_ptr);
      }
    }
  };
  struct ComponentInit final : public ComponentData
  {
    ComponentId componentId;

    template<typename T>
    ComponentInit(ComponentId component_id, T &&_data) : ComponentData(std::move(_data)), componentId(component_id) {}

    template<typename T>
    ComponentInit(const char *component_name, T &&_data) :
        ComponentData(std::move(_data)), componentId(get_component_id(ecs::TypeInfo<T>::typeId, component_name)) {}

    template<typename T>
    ComponentInit(EcsManager &manager, const char *component_name, T &&_data) :
        ComponentData(std::move(_data)), componentId(get_or_add_component(manager, ecs::TypeInfo<T>::typeId, component_name)) {}


    ComponentInit(ComponentInit &&other) : ComponentData(static_cast<ComponentData&&>(std::move(other))), componentId(other.componentId)
    {
      other.componentId = {};
    }

    ComponentInit(const ComponentInit &other) = delete;

    ComponentInit &operator=(const ComponentInit &other) = delete;
  };

  struct ComponentSoaInit final : public ComponentDataSoa
  {
    ComponentId componentId;

    template<typename T>
    ComponentSoaInit(ComponentId component_id, std::vector<T> &&_data) : ComponentDataSoa(std::move(_data)), componentId(component_id) {}

    template<typename T>
    ComponentSoaInit(const char *component_name, std::vector<T> &&_data) :
        ComponentDataSoa(std::move(_data)), componentId(get_component_id(ecs::TypeInfo<T>::typeId, component_name)) {}

    template<typename T>
    ComponentSoaInit(EcsManager &manager, const char *component_name, std::vector<T> &&_data) :
        ComponentDataSoa(std::move(_data)), componentId(get_or_add_component(manager, ecs::TypeInfo<T>::typeId, component_name)) {}

    ComponentSoaInit(ComponentSoaInit &&other) : ComponentDataSoa(std::move(other)), componentId(other.componentId)
    {
      other.componentId = {};
    }

    ComponentSoaInit(const ComponentSoaInit &other) = delete;

    ComponentSoaInit &operator=(const ComponentSoaInit &other) = delete;
  };

  struct InitializerList
  {
    ska::flat_hash_map<ComponentId, ecs::ComponentData> args;

    InitializerList() = default;

    template<size_t N>
    InitializerList(ecs::ComponentInit (&&_args)[N])
    {
      args.reserve(N + 1 /*entityId*/);
      for (size_t i = 0; i < N; i++)
      {
        args.insert({_args[i].componentId, std::move(static_cast<ComponentData &&>(_args[i]))});
      }
    }

    void push_back(ecs::ComponentInit &&componentInit)
    {
      args.insert({componentInit.componentId, std::move(static_cast<ComponentData &&>(componentInit))});
    }

    void reserve(size_t count)
    {
      args.reserve(count + 1 /*entityId*/);
    }

    size_t size() const
    {
      return args.size();
    }
    void clear()
    {
      args.clear();
    }
  };

  struct InitializerSoaList
  {
    ska::flat_hash_map<ComponentId, ecs::ComponentDataSoa> args;
    int m_size = 0;
    InitializerSoaList() = default;

    template<size_t N>
    InitializerSoaList(ecs::ComponentSoaInit (&&_args)[N])
    {
      args.reserve(N + 1 /*entityId*/);
      for (size_t i = 0; i < N; i++)
      {
        m_size = m_size == 0 ? _args[i].size() : std::min(m_size, (int)_args[i].size());
        args.insert({_args[i].componentId, std::move(static_cast<ComponentDataSoa &&>(_args[i]))});
      }
    }

    void push_back(ecs::ComponentSoaInit &&componentInit)
    {
      if (componentInit.size() == 0)
        return;
      m_size = m_size == 0 ? componentInit.size() : std::min(m_size, (int)componentInit.size());
      args.insert({componentInit.componentId, std::move(static_cast<ComponentDataSoa &&>(componentInit))});
    }
    void reserve(size_t count)
    {
      args.reserve(count + 1 /*entityId*/);
    }

    size_t size() const
    {
      return m_size;
    }
  };
}