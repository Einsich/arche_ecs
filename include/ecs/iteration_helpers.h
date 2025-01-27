#pragma once

#include "ecs/config.h"
#include "ecs/ecs_manager.h"

namespace ecs_details
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

  static PrtWrapper<T> cast(std::vector<char *> *ptr, uint32_t idx) { return PrtWrapper<T>(ptr ? (T *)((*ptr)[idx]) : nullptr); }
};


template<typename T>
struct SingletonWrapper
{
  T *ptr;
  SingletonWrapper(T *ptr) : ptr(ptr) {}
  T &operator*() { return *ptr; }
  T *operator[](int) { return ptr; }
  void operator++() { }
};

template<typename T>
struct Ptr
{
  static auto cast(std::vector<char *> *ptr, uint32_t idx)
  {
    if constexpr (!ecs::TypeInfo<typename std::remove_const<T>::type>::isSingleton)
    {
      return ptr ? (T *)((*ptr)[idx]) : nullptr;
    }
    else
    {
      return SingletonWrapper<T>((T*)ptr);
    }
  }
};

template<typename T>
struct restrict_type;

template<typename T>
struct restrict_type<Ptr<T>>
{
  using type = T *__restrict__;
};

template<typename T>
struct restrict_type<PrtWrapper<T>>
{
  using type = PrtWrapper<T>;
};

template<typename T>
struct restrict_type<SingletonWrapper<T>>
{
  using type = SingletonWrapper<T>;
};

template<size_t UNROLL_N, typename ...PtrArgs, typename Callable>
static void query_chunk_iteration(Callable &&callable_query, uint32_t entities_count, PtrArgs ...components)
{
  #pragma unroll UNROLL_N
  for (uint32_t i = 0; i < entities_count; i++)
  {
    callable_query((*components)...);
    ((++components), ...);
  }
}

template<size_t N, typename ...CastArgs, typename Callable, std::size_t... I>
static void query_archetype_iteration(ecs_details::Archetype &archetype, const ecs::ToComponentMap &chunks, Callable &&callable_query, std::index_sequence<I...>)
{
  for (uint32_t chunkIdx = 0, chunkCount = archetype.chunkCount, entityOffset = 0; chunkIdx < chunkCount; chunkIdx++, entityOffset += archetype.chunkSize)
  {
    uint32_t entitiesCount = std::min(archetype.entityCount - entityOffset, archetype.chunkSize);
    query_chunk_iteration<4>(std::move(callable_query), entitiesCount, CastArgs::cast(chunks[I], chunkIdx)...);
  }
}

template<size_t UNROLL_N, typename ...PtrArgs, typename E, typename Callable>
static void event_chunk_iteration(E &&event, Callable &&callable_query, uint32_t entities_count, typename restrict_type<PtrArgs>::type ...components)
{
  #pragma unroll UNROLL_N
  for (uint32_t i = 0; i < entities_count; i++)
  {
    callable_query(event, (*components)...);
    ((++components), ...);
  }
}

template<size_t N, typename ...CastArgs, typename E, typename Callable, std::size_t... I>
static void event_archetype_iteration(ecs_details::Archetype &archetype, const ecs::ToComponentMap &chunks, E &&event, Callable &&callable_query, std::index_sequence<I...>)
{
  for (uint32_t chunkIdx = 0, chunkCount = archetype.chunkCount, entityOffset = 0; chunkIdx < chunkCount; chunkIdx++, entityOffset += archetype.chunkSize)
  {
    uint32_t entitiesCount = std::min(archetype.entityCount - entityOffset, archetype.chunkSize);
    event_chunk_iteration<4, CastArgs...>(std::forward<E>(event), std::move(callable_query), entitiesCount, CastArgs::cast(chunks[I], chunkIdx)...);
  }
}

template<size_t N, typename ...CastArgs, typename E, typename Callable, std::size_t... I>
static void event_invoke_for_entity(ecs_details::Archetype &archetype, const ecs::ToComponentMap &chunks, uint32_t component_idx, E &&event, Callable &&callable_query, std::index_sequence<I...>)
{
  uint32_t chunkIdx = component_idx >> archetype.chunkSizePower;
  uint32_t offsetInChunk = component_idx & archetype.chunkMask;
  callable_query(event, ((CastArgs::cast(chunks[I], chunkIdx))[offsetInChunk])...);
}

template<size_t N, typename ...CastArgs, typename Callable>
static void query_iteration(ecs::EcsManager &mgr, ecs::NameHash query_hash, Callable &&query_function)
{
  auto it = mgr.queries.find(query_hash);
  if (it != mgr.queries.end())
  {
    ecs::Query &query = it->second;
    for (const auto &[archetypeId, archetypeRecord] : query.archetypesCache)
    {
      ecs_details::Archetype &archetype = *archetypeRecord.archetype;
      const ecs::ToComponentMap &toComponentIndex = archetypeRecord.toComponentIndex;
      ecs::mark_dirty(archetype, archetypeRecord.toTrackedComponent);
      query_archetype_iteration<N, CastArgs...>(archetype, toComponentIndex, std::move(query_function), std::make_index_sequence<N>());
    }
  }
}

template<size_t N, typename ...CastArgs, typename Callable, std::size_t... I>
static void query_invoke_for_entity_impl(ecs_details::Archetype &archetype, const ecs::ToComponentMap &chunks, uint32_t component_idx, Callable &&callable_query, std::index_sequence<I...>)
{
  uint32_t chunkIdx = component_idx >> archetype.chunkSizePower;
  uint32_t offsetInChunk = component_idx & archetype.chunkMask;
  callable_query(((CastArgs::cast(chunks[I], chunkIdx))[offsetInChunk])...);
}

template<size_t N, typename ...CastArgs, typename Callable>
static void query_invoke_for_entity(ecs::EcsManager &mgr, ecs::EntityId eid, ecs::NameHash query_hash, Callable &&query_function)
{
  auto it = mgr.queries.find(query_hash);
  if (it != mgr.queries.end())
  {
    ecs::Query &query = it->second;
    ecs::ArchetypeId archetypeId;
    uint32_t componentIdx;
    if (mgr.entityContainer.get(eid, archetypeId, componentIdx))
    {
      auto ait = query.archetypesCache.find(archetypeId);
      if (ait != query.archetypesCache.end())
      {
        const ecs::ArchetypeRecord &archetypeRecord = ait->second;
        ecs_details::Archetype &archetype = *archetypeRecord.archetype;
        const ecs::ToComponentMap &toComponentIndex = archetypeRecord.toComponentIndex;
        ecs::mark_dirty(archetype, archetypeRecord.toTrackedComponent, componentIdx);
        query_invoke_for_entity_impl<N, CastArgs...>(archetype, toComponentIndex, componentIdx, std::move(query_function), std::make_index_sequence<N>());
      }
    }
  }
}

} // namespace ecs
