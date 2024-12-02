#pragma once
#include "ecs/config.h"

namespace ecs
{
  struct ComponentData
  {
    void *data;

    void (*destructor)(void *data);
    void *(*copy_constructor)(const void *src_data);

    template <typename T>
    static void Destructor(void *data)
    {
      if (data)
      {
        ((T *)data)->~T();
        delete (T *)data;
      }
    }
    template <typename T>
    static void * CopyConstructor(const void *src_data)
    {
      return new T(*(T *)src_data);
    }

    ComponentData() = default;

    template<typename T>
    ComponentData(T &&_data) : data(new T(std::forward<T>(_data))), destructor(Destructor<T>), copy_constructor(CopyConstructor<T>) {}

    ComponentData(ComponentData &&other) : data(other.data), destructor(other.destructor), copy_constructor(other.copy_constructor)
    {
      other.data = nullptr;
    }

    ComponentData copy() const
    {
      void *new_data = copy_constructor(data);
      ComponentData copy;
      copy.data = new_data;
      copy.destructor = destructor;
      copy.copy_constructor = copy_constructor;
      return copy;
    }

    ComponentData &operator=(ComponentData &&other)
    {
      if (this != &other)
      {
        this->~ComponentData();
        data = other.data;
        destructor = other.destructor;
        other.data = nullptr;
      }
      return *this;
    }

    ComponentData(const ComponentData &other) = delete;

    ComponentData &operator=(const ComponentData &other) = delete;

    ~ComponentData()
    {
      destructor(data);
    }
  };

  struct ComponentDataSoa
  {
    std::vector<void *> data;

    void (*destructor)(void *data);

    template <typename T>
    static void Destructor(void *data)
    {
      if (data)
      {
        ((T *)data)->~T();
        delete (T *)data;
      }
    }
    template<typename T>
    ComponentDataSoa(std::vector<T> &&_data) : destructor(Destructor<T>)
    {
      data.resize(_data.size());
      for (size_t i = 0; i < _data.size(); i++)
      {
        data[i] = new T(std::forward<T>(_data[i]));
      }
    }
    ComponentDataSoa(ComponentDataSoa &&other) : data(std::move(other.data)), destructor(other.destructor) {}
    ComponentDataSoa &operator=(ComponentDataSoa &&other)
    {
      if (this != &other)
      {
        this->~ComponentDataSoa();
        data = std::move(other.data);
        destructor = other.destructor;
      }
      return *this;
    }
    ~ComponentDataSoa()
    {
      for (void *ptr : data)
      {
        destructor(ptr);
      }
    }


  };
  struct ComponentInit final : public ComponentData
  {
    ComponentId componentId;

    template<typename T>
    ComponentInit(ComponentId component_id, T &&_data) : ComponentData(std::forward<T>(_data)), componentId(component_id) {}

    template<typename T>
    ComponentInit(const char *component_name, T &&_data) :
        ComponentData(std::forward<T>(_data)), componentId(get_component_id(ecs::TypeInfo<T>::typeId, component_name)) {}


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
        m_size = m_size == 0 ? _args[i].data.size() : std::min(m_size, (int)_args[i].data.size());
        args.insert({_args[i].componentId, std::move(static_cast<ComponentDataSoa &&>(_args[i]))});
      }
    }

    void push_back(ecs::ComponentSoaInit &&componentInit)
    {
      if (componentInit.data.size() == 0)
        return;
      m_size = m_size == 0 ? componentInit.data.size() : std::min(m_size, (int)componentInit.data.size());
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