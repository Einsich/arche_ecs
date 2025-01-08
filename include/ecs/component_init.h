#pragma once
#include "ecs/config.h"
#include "ecs/any.h"

namespace ecs
{

  struct EcsManager;
  ComponentId get_or_add_component(EcsManager &manager, TypeId typeId, const char *component_name);


  struct ComponentDataSoa
  {
    void *vector_ptr;
    void *vector_data_ptr;
    uint32_t m_size;
    uint32_t m_sizeof;
    TypeId typeId;

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
    ComponentDataSoa(std::vector<T> &&_data, TypeId type_id) : typeId(type_id), destructor(Destructor<std::vector<T>>)
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
      typeId(other.typeId),
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

  struct LazyInit {};

  struct ComponentInit final : public ecs::Any
  {
    ComponentId componentId;

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    ComponentInit(ComponentId component_id, ValueType &&_data) :
        ecs::Any(std::move(_data), ecs::TypeInfo<T>::typeId), componentId(component_id) {}

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    ComponentInit(const char *component_name, ValueType &&_data) :
        ComponentInit(get_component_id(ecs::TypeInfo<T>::typeId, component_name), std::move(_data)) {}

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    ComponentInit(EcsManager &manager, const char *component_name, ValueType &&_data) :
        ComponentInit(get_or_add_component(manager, ecs::TypeInfo<T>::typeId, component_name), std::move(_data)) {}

    template<typename ValueType>
    ComponentInit(ComponentId component_id, const ValueType&_data) :
        ecs::Any(_data, ecs::TypeInfo<T>::typeId), componentId(component_id) {}

    template<typename ValueType>
    ComponentInit(const char *component_name, const ValueType &_data) :
        ComponentInit(get_component_id(ecs::TypeInfo<T>::typeId, component_name), _data) {}

    template<typename ValueType>
    ComponentInit(EcsManager &manager, const char *component_name, const ValueType &_data) :
        ComponentInit(get_or_add_component(manager, ecs::TypeInfo<T>::typeId, component_name), _data) {}


    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    ComponentInit(ComponentId component_id, ecs::LazyInit, ValueType &&_data) :
        ecs::Any(std::move(_data), ecs::LazyInitTypeBind<T>::typeId), componentId(component_id) {}

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    ComponentInit(const char *component_name, ecs::LazyInit, ValueType &&_data) :
        ComponentInit(get_component_id(ecs::LazyInitTypeBind<T>::typeId, component_name), std::move(_data)) {}

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    ComponentInit(EcsManager &manager, const char *component_name, ecs::LazyInit, ValueType &&_data) :
        ComponentInit(get_or_add_component(manager, ecs::LazyInitTypeBind<T>::typeId, component_name), std::move(_data)) {}

    template<typename ValueType>
    ComponentInit(ComponentId component_id, ecs::LazyInit, const ValueType&_data) :
        ecs::Any(_data, ecs::LazyInitTypeBind<T>::typeId), componentId(component_id) {}

    template<typename ValueType>
    ComponentInit(const char *component_name, ecs::LazyInit, const ValueType &_data) :
        ComponentInit(get_component_id(ecs::LazyInitTypeBind<T>::typeId, component_name), _data) {}

    template<typename ValueType>
    ComponentInit(EcsManager &manager, const char *component_name, ecs::LazyInit, const ValueType &_data) :
        ComponentInit(get_or_add_component(manager, ecs::LazyInitTypeBind<T>::typeId, component_name), _data) {}

    ComponentInit(ComponentId component_id, ecs::Any &&_data) :
        ecs::Any(std::move(_data)), componentId(component_id) {}

    ComponentInit(ComponentInit &&other) : ecs::Any(static_cast<ecs::Any&&>(std::move(other))), componentId(other.componentId)
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
    ComponentSoaInit(ComponentId component_id, std::vector<T> &&_data) : ComponentDataSoa(std::move(_data), ecs::TypeInfo<T>::typeId), componentId(component_id) {}

    template<typename T>
    ComponentSoaInit(const char *component_name, std::vector<T> &&_data) :
        ComponentDataSoa(std::move(_data), ecs::TypeInfo<T>::typeId), componentId(get_component_id(ecs::TypeInfo<T>::typeId, component_name)) {}

    template<typename T>
    ComponentSoaInit(EcsManager &manager, const char *component_name, std::vector<T> &&_data) :
        ComponentDataSoa(std::move(_data), ecs::TypeInfo<T>::typeId), componentId(get_or_add_component(manager, ecs::TypeInfo<T>::typeId, component_name)) {}

    ComponentSoaInit(ComponentSoaInit &&other) : ComponentDataSoa(std::move(other)), componentId(other.componentId)
    {
      other.componentId = {};
    }

    ComponentSoaInit(const ComponentSoaInit &other) = delete;

    ComponentSoaInit &operator=(const ComponentSoaInit &other) = delete;
  };

  struct InitializerList
  {
    ska::flat_hash_map<ComponentId, ecs::Any> args;

    InitializerList() = default;

    template<size_t N>
    InitializerList(ecs::ComponentInit (&&_args)[N])
    {
      args.reserve(N + 1 /*entityId*/);
      for (size_t i = 0; i < N; i++)
      {
        args.insert({_args[i].componentId, std::move(static_cast<ecs::Any &&>(_args[i]))});
      }
    }

    void push_back(ecs::ComponentInit &&componentInit)
    {
      args.insert({componentInit.componentId, std::move(static_cast<ecs::Any &&>(componentInit))});
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