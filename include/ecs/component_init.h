#pragma once
#include "ecs/config.h"
#include "ecs/any.h"
#include "ecs/type_declaration_helper.h"
namespace ecs
{
  struct EcsManager;
}
namespace ecs_details
{
  using InitializerListType = ska::flat_hash_map<ecs::ComponentId, ecs::Any>;
  InitializerListType get_init_list(ecs::EcsManager &mgr);
} // namespace ecs_details

namespace ecs
{
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

  struct ComponentInit final : public ecs::AnyComponent
  {

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    ComponentInit(ComponentId component_id, ValueType &&_data) :
        ecs::Any(std::move(_data), ecs::TypeInfo<T>::typeId, component_id) {}

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    ComponentInit(const char *component_name, ValueType &&_data) :
        ComponentInit(get_component_id(ecs::TypeInfo<T>::typeId, component_name), std::move(_data)) {}

    template<typename ValueType, typename T = std::remove_cvref<ValueType>::type>
    ComponentInit(EcsManager &manager, const char *component_name, ValueType &&_data) :
        ComponentInit(get_or_add_component<T>(manager, component_name), std::move(_data)) {}

    template<typename ValueType>
    ComponentInit(ComponentId component_id, const ValueType&_data) :
        ecs::Any(_data, ecs::TypeInfo<T>::typeId, component_id) {}

    template<typename ValueType>
    ComponentInit(const char *component_name, const ValueType &_data) :
        ComponentInit(get_component_id(ecs::TypeInfo<T>::typeId, component_name), _data) {}

    template<typename ValueType>
    ComponentInit(EcsManager &manager, const char *component_name, const ValueType &_data) :
        ComponentInit(get_or_add_component<T>(manager, component_name), _data) {}


    ComponentInit(ecs::Any &&_data) :
        ecs::Any(std::move(_data)) {}

    ComponentInit(ComponentInit &&other) : ecs::Any(static_cast<ecs::Any&&>(std::move(other)))
    {
    }

    ComponentInit(const ComponentInit &other) = delete;

    ComponentInit &operator=(const ComponentInit &other) = delete;
  };

  static_assert(sizeof(ComponentInit) == 64);

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
        ComponentDataSoa(std::move(_data), ecs::TypeInfo<T>::typeId), componentId(get_or_add_component<T>(manager, component_name)) {}

    ComponentSoaInit(ComponentSoaInit &&other) : ComponentDataSoa(std::move(other)), componentId(other.componentId)
    {
      other.componentId = {};
    }

    ComponentSoaInit(const ComponentSoaInit &other) = delete;

    ComponentSoaInit &operator=(const ComponentSoaInit &other) = delete;
  };

  struct InitializerList
  {
    using type = ska::flat_hash_map<ComponentId, ecs::Any>;
    type args;

    struct Empty {};
    InitializerList(Empty) {}

    InitializerList(ecs::EcsManager &mgr) : args(ecs_details::get_init_list(mgr)) {}

    template<size_t N>
    InitializerList(ecs::EcsManager &mgr, ecs::ComponentInit (&&_args)[N]) : args(ecs_details::get_init_list(mgr))
    {
      args.reserve(N + 1 /*entityId*/);
      for (size_t i = 0; i < N; i++)
      {
        args.emplace(_args[i].componentId, std::move(static_cast<ecs::Any &&>(_args[i])));
      }
    }
    InitializerList(InitializerList &&other) : args(std::move(other.args)) {}
    InitializerList &operator=(InitializerList &&other)
    {
      args = std::move(other.args);
      return *this;
    }
    InitializerList(const InitializerList &other) = delete;
    InitializerList &operator=(const InitializerList &other) = delete;

    void push_back(ecs::ComponentInit &&componentInit)
    {
      args.emplace(componentInit.componentId, std::move(static_cast<ecs::Any &&>(componentInit)));
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