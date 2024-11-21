#pragma once
#include "ecs/config.h"

namespace ecs
{
  struct ComponentInit
  {
    ComponentId componentId;

    void *data;

    void (*destructor)(void *data);

    template <typename T>
    static void ComponentInitDestruct(void *data)
    {
      if (data)
      {
        ((T *)data)->~T();
        delete (T *)data;
      }
    }

    template<typename T>
    ComponentInit(ComponentId _componentId, T &&_data) : componentId(_componentId), data(new T(std::forward<T>(_data))),
    destructor(ComponentInitDestruct<T>) {}

    ComponentInit(ComponentInit &&other) : componentId(other.componentId), data(other.data), destructor(other.destructor)
    {
      other.data = nullptr;
    }

    ComponentInit(const ComponentInit &other) = delete;

    ComponentInit &operator=(const ComponentInit &other) = delete;

    ~ComponentInit()
    {
      destructor(data);
    }
  };

  struct InitializerList
  {

    std::vector<ecs::ComponentInit> args;

    InitializerList() = default;

    template<size_t N>
    InitializerList(ecs::ComponentInit (&&_args)[N])
    {
      args.reserve(N);
      for (size_t i = 0; i < N; i++)
      {
        args.push_back(std::move(_args[i]));
      }
    }

    void push_back(ecs::ComponentInit &&componentInit)
    {
      args.push_back(std::move(componentInit));
    }

    void reserve(size_t count)
    {
      args.reserve(count);
    }

    size_t size() const
    {
      return args.size();
    }
  };
}