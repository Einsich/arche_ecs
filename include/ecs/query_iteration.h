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
  T *operator[](int idx) { return ptr + idx; }
  void operator++() { ptr = ptr ? ptr + 1 : ptr; }
  void operator+=(int x) { ptr = ptr ? ptr + x : ptr; }
};
template<size_t UNROLL_N, typename Callable, typename ...NoCRefPtrArgs>
void perform_system_unroll(Callable &&callable_query, uint32_t elementCount, NoCRefPtrArgs ...components)
{
  #pragma unroll UNROLL_N
  for (uint32_t i = 0; i < elementCount; i++)
  {
    callable_query((*components)...);
    ((++components), ...);
  }
}

template<typename Callable, typename ...NoCRefPtrArgs>
void perform_system(Callable &&callable_query, uint32_t elementCount, NoCRefPtrArgs ...components)
{
  // #pragma unroll 4
  for (uint32_t i = 0; i < elementCount; i++)
  {
    callable_query((*components)...);
    ((++components), ...);
  }
}

template<size_t N, typename ...CastArgs, typename Callable, std::size_t... I>
void templated_archetype_iterate(ecs::Archetype &archetype, const ecs::ToComponentMap &chunks, Callable &&callable_query, std::index_sequence<I...>)
{
  int componenIdx[N] = {to_archetype_component[I]...};

  char **chunks[N] = {
    (componenIdx[I] >= 0 ? archetype.collumns[componenIdx[I]].chunks.data() : nullptr)...
  };

  for (uint32_t chunkIdx = 0, chunkCount = archetype.chunkCount; chunkIdx < chunkCount; chunkIdx++)
  {
    uint32_t elementCount = std::min(archetype.entityCount - archetype.chunkSize * chunkIdx, archetype.chunkSize);
    perform_system_unroll<4>(std::move(callable_query), elementCount, (CastArgs)(chunks[I] ? chunks[I][chunkIdx] : nullptr)...);
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
