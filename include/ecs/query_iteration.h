#pragma once

#include "ecs/config.h"
#include "ecs/ecs_manager.h"

namespace ecs
{

template<typename T>
struct PrtWrapper
{
  T *ptr;
  PrtWrapper(T *ptr) : ptr(ptr) {}
  PrtWrapper(char *ptr) : ptr((T*)ptr) {}
  T *operator*() { return ptr; }
  void operator++() { ptr = ptr ? ptr + 1 : ptr; }
};

template<typename Callable, typename ...NoCRefPtrArgs>
void perform_system(Callable &&callable_query, size_t elementCount, NoCRefPtrArgs ...components)
{
  for (size_t i = 0; i < elementCount; i++)
  {
    callable_query(*(components)...);
    ((++components), ...);
  }
}

template<size_t N, typename ...CastArgs, typename Callable, std::size_t... I>
void templated_archetype_iterate(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, Callable &&callable_query, std::index_sequence<I...>)
{
  int componenIdx[N] = {to_archetype_component[I]...};

  char **chunks[N] = {
    (componenIdx[I] >= 0 ? archetype.collumns[componenIdx[I]].chunks.data() : nullptr)...
  };

  for (uint32_t chunkIdx = 0, chunkCount = archetype.chunkCount; chunkIdx < chunkCount; chunkIdx++)
  {
    uint32_t elementCount = std::min(archetype.entityCount - archetype.chunkSize * chunkIdx, archetype.chunkSize);
    perform_system(std::move(callable_query), elementCount, (CastArgs)(chunks[I] ? chunks[I][chunkIdx] : nullptr)...);
  }
}

template<typename QueryImpl>
void call_query(ecs::EcsManager &mgr, ecs::NameHash query_hash, QueryImpl &&query_impl)
{
  auto it = mgr.queries.find(query_hash);
  if (it != mgr.queries.end())
  {
    ecs::Query &query = it->second;
    for (const auto &archetypeRecord : query.archetypesCache)
    {
      auto it = mgr.archetypeMap.find(archetypeRecord.archetypeId);
      if (it == mgr.archetypeMap.end())
      {
        continue;
      }
      ecs::Archetype &archetype = it->second;
      const ecs::ToComponentMap &toComponentIndex = archetypeRecord.toComponentIndex;
      query_impl(archetype, toComponentIndex);
    }
  }
}

} // namespace ecs
