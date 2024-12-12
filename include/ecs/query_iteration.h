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
template <typename T>
struct restrict_type
{
  using type = T __restrict__;
};
template<typename T>
struct restrict_type<PrtWrapper<T>>
{
  using type = PrtWrapper<T>;
};

template<size_t UNROLL_N, typename ...PtrArgs, typename Callable>
void perform_system_unroll(Callable &&callable_query, uint32_t elementCount, typename restrict_type<PtrArgs>::type ...components)
{
  #pragma unroll UNROLL_N
  for (uint32_t i = 0; i < elementCount; i++)
  {
    callable_query((*components)...);
    ((++components), ...);
  }
}


template<size_t N, typename ...CastArgs, typename Callable, std::size_t... I>
void templated_archetype_iterate(ecs::Archetype &archetype, const ecs::ToComponentMap &chunks, Callable &&callable_query, std::index_sequence<I...>)
{
  for (uint32_t chunkIdx = 0, chunkCount = archetype.chunkCount, entityOffset = 0; chunkIdx < chunkCount; chunkIdx++, entityOffset += archetype.chunkSize)
  {
    uint32_t elementCount = std::min(archetype.entityCount - entityOffset, archetype.chunkSize);
    perform_system_unroll<4, CastArgs...>(std::move(callable_query), elementCount, (CastArgs)(chunks[I] ? (*chunks[I])[chunkIdx] : nullptr)...);
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
      ecs::Archetype &archetype = *archetypeRecord.archetype;
      const ecs::ToComponentMap &toComponentIndex = archetypeRecord.toComponentIndex;
      query_impl(archetype, toComponentIndex);
    }
  }
}

} // namespace ecs
