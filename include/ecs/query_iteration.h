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
  T *operator[](int idx) { return ptr ? ptr + idx : ptr; }
  void operator++() { ptr = ptr ? ptr + 1 : ptr; }
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
void perform_system_unroll(Callable &&callable_query, uint32_t entities_count, typename restrict_type<PtrArgs>::type ...components)
{
  #pragma unroll UNROLL_N
  for (uint32_t i = 0; i < entities_count; i++)
  {
    callable_query((*components)...);
    ((++components), ...);
  }
}

template<size_t UNROLL_N, typename ...PtrArgs, typename E, typename Callable>
void perform_event_unroll(E &&event, Callable &&callable_query, uint32_t entities_count, typename restrict_type<PtrArgs>::type ...components)
{
  #pragma unroll UNROLL_N
  for (uint32_t i = 0; i < entities_count; i++)
  {
    callable_query(event, (*components)...);
    ((++components), ...);
  }
}


template<size_t N, typename ...CastArgs, typename Callable, std::size_t... I>
void templated_archetype_iterate(ecs::Archetype &archetype, const ecs::ToComponentMap &chunks, Callable &&callable_query, std::index_sequence<I...>)
{
  for (uint32_t chunkIdx = 0, chunkCount = archetype.chunkCount, entityOffset = 0; chunkIdx < chunkCount; chunkIdx++, entityOffset += archetype.chunkSize)
  {
    uint32_t entitiesCount = std::min(archetype.entityCount - entityOffset, archetype.chunkSize);
    perform_system_unroll<4, CastArgs...>(std::move(callable_query), entitiesCount, (CastArgs)(chunks[I] ? (*chunks[I])[chunkIdx] : nullptr)...);
  }
}


template<size_t N, typename ...CastArgs, typename E, typename Callable, std::size_t... I>
void templated_event_archetype_iterate(ecs::Archetype &archetype, const ecs::ToComponentMap &chunks, E &&event, Callable &&callable_query, std::index_sequence<I...>)
{
  for (uint32_t chunkIdx = 0, chunkCount = archetype.chunkCount, entityOffset = 0; chunkIdx < chunkCount; chunkIdx++, entityOffset += archetype.chunkSize)
  {
    uint32_t entitiesCount = std::min(archetype.entityCount - entityOffset, archetype.chunkSize);
    perform_event_unroll<4, CastArgs...>(std::forward<E>(event), std::move(callable_query), entitiesCount, (CastArgs)(chunks[I] ? (*chunks[I])[chunkIdx] : nullptr)...);
  }
}

template<size_t N, typename ...CastArgs, typename E, typename Callable, std::size_t... I>
void templated_archetype_event_one_entity(ecs::Archetype &archetype, const ecs::ToComponentMap &chunks, uint32_t component_idx, E &&event, Callable &&callable_query, std::index_sequence<I...>)
{
  uint32_t chunkIdx = component_idx >> archetype.chunkSizePower;
  uint32_t offsetInChunk = component_idx & archetype.chunkMask;
  callable_query(event, (((CastArgs)(chunks[I] ? (*chunks[I])[chunkIdx] : nullptr))[offsetInChunk])...);
}

template<size_t N, typename ...CastArgs, typename Callable>
void call_query(ecs::EcsManager &mgr, ecs::NameHash query_hash, Callable &&query_function)
{
  auto it = mgr.queries.find(query_hash);
  if (it != mgr.queries.end())
  {
    ecs::Query &query = it->second;
    for (const auto &[archetypeId, archetypeRecord] : query.archetypesCache)
    {
      ecs::Archetype &archetype = *archetypeRecord.archetype;
      const ecs::ToComponentMap &toComponentIndex = archetypeRecord.toComponentIndex;
      templated_archetype_iterate<N, CastArgs...>(archetype, toComponentIndex, std::move(query_function), std::make_index_sequence<N>());
    }
  }
}

template<size_t N, typename ...CastArgs, typename Callable, std::size_t... I>
void templated_archetype_one_entity(ecs::Archetype &archetype, const ecs::ToComponentMap &chunks, uint32_t component_idx, Callable &&callable_query, std::index_sequence<I...>)
{
  uint32_t chunkIdx = component_idx >> archetype.chunkSizePower;
  uint32_t offsetInChunk = component_idx & archetype.chunkMask;
  callable_query((((CastArgs)(chunks[I] ? (*chunks[I])[chunkIdx] : nullptr))[offsetInChunk])...);
}

template<size_t N, typename ...CastArgs, typename Callable>
void call_query(ecs::EcsManager &mgr, ecs::EntityId eid, ecs::NameHash query_hash, Callable &&query_function)
{
  auto it = mgr.queries.find(query_hash);
  if (it != mgr.queries.end())
  {
    ecs::Query &query = it->second;
    ArchetypeId archetypeId;
    uint32_t componentIdx;
    if (mgr.entityContainer.get(eid, archetypeId, componentIdx))
    {
      auto ait = query.archetypesCache.find(archetypeId);
      if (ait != query.archetypesCache.end())
      {
        const auto &archetypeRecord = ait->second;
        ecs::Archetype &archetype = *archetypeRecord.archetype;
        const ecs::ToComponentMap &toComponentIndex = archetypeRecord.toComponentIndex;
        templated_archetype_one_entity<N, CastArgs...>(archetype, toComponentIndex, componentIdx, std::move(query_function), std::make_index_sequence<N>());
      }
    }
  }
}

} // namespace ecs
