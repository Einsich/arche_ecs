#include "ecs/query.h"
#include "ecs/ecs_manager.h"
#include <assert.h>

namespace ecs_details
{
bool try_registrate_track(ecs::EcsManager &mgr, const std::vector<ecs::ComponentId> &tracked_components, ecs_details::Archetype &archetype, ecs::NameHash event_hash);
}

namespace ecs
{

enum class NodeState
{
  Black,
  Gray,
  White,
  Cycle
};

using Edges = std::vector<std::vector<uint32_t>>;

static bool gather_cycle_nodes(uint32_t c, uint32_t v, const Edges &edges, const std::vector<NodeState> &used, std::vector<uint32_t> &cycle)
{
  if (c == v)
    return true;
  for (uint32_t to : edges[v])
  {
    if (used[to] == NodeState::Gray)
    {
      if (gather_cycle_nodes(c, to, edges, used, cycle))
      {
        cycle.push_back(to);
        return true;
      }
    }
  }
  return false;
}

template <typename Loger>
static void dfs(uint32_t v, const Edges &edges, std::vector<NodeState> &used, std::vector<uint32_t> &answer, const Loger &loger)
{
  used[v] = NodeState::Gray;
  for (uint32_t to : edges[v])
  {
    if (used[to] == NodeState::Gray)
    {
      std::vector<uint32_t> cycle;
      gather_cycle_nodes(v, to, edges, used, cycle);
      cycle.push_back(to);
      loger(cycle);
      used[to] = NodeState::Cycle;
      continue;
    }
    if (used[to] == NodeState::Black)
      dfs(to, edges, used, answer, loger);
  }
  answer.push_back(v);
  used[v] = NodeState::White;
}

template <typename GetNode>
static std::vector<uint32_t> topological_sort(EcsManager &mgr, uint32_t nodeCount, const GetNode &get_node)
{
  Edges edge(nodeCount);
  std::vector<NodeState> used(nodeCount, NodeState::Black);
  std::vector<uint32_t> answer;
  answer.reserve(nodeCount);

  ska::flat_hash_map<ecs_details::tiny_string, std::vector<uint32_t>> nameMap;
  for (uint32_t i = 0; i < nodeCount; i++)
    nameMap[get_node(i)->name].push_back(i);

  for (uint32_t i = 0; i < nodeCount; i++)
  {
    const auto &query = *get_node(i);
    for (const ecs_details::tiny_string &before : query.before)
    {
      auto it = nameMap.find(before);
      if (it != nameMap.end())
      {
        for (uint32_t j : it->second)
        {
          if (i == j)
          {
            ECS_LOG_ERROR(mgr).log("system %s has itself in before", query.name.c_str());
            continue;
          }
          edge[i].push_back(j);
        }
      }
      else
      {
        ECS_LOG_ERROR(mgr).log("system %s doesn't exist for before %s", before.c_str(), query.name.c_str());
      }
    }
    for (const ecs_details::tiny_string &after : query.after)
    {
      auto it = nameMap.find(after);
      if (it != nameMap.end())
      {
        for (uint32_t j : it->second)
        {
          if (i == j)
          {
            ECS_LOG_ERROR(mgr).log("system %s has itself in after", query.name.c_str());
            continue;
          }
          edge[j].push_back(i);
        }
      }
      else
      {
        ECS_LOG_ERROR(mgr).log("system %s doesn't exist for after %s", after.c_str(), query.name.c_str());
      }
    }
  }
  const auto loger = [&](const std::vector<uint32_t> &nodes) {

    constexpr int MAX_BUFFER_SIZE = 1024;
    char buff[MAX_BUFFER_SIZE];
    int length = 0;
    length += snprintf(buff + length, MAX_BUFFER_SIZE - length, "cycle detected : \n");
    for (uint32_t i : nodes)
      length += snprintf(buff + length, MAX_BUFFER_SIZE - length, "%s -> ", get_node(i)->name.c_str());

    ECS_LOG_ERROR(mgr).log(buff);
  };
  for (uint32_t i = 0; i < nodeCount; i++)
  {
    if (used[i] == NodeState::Black)
      dfs(i, edge, used, answer, loger);
  }
  return answer;
}

template <typename T>
void apply_reorder(std::vector<T> &vec, const std::vector<uint32_t> &order)
{
  std::vector<T> tmp(order.size());
  for (uint32_t i = 0; i < order.size(); i++)
    tmp[i] = std::move(vec[order[i]]);
  vec.swap(tmp);
}

void sort_systems(EcsManager &mgr)
{
  {
    std::vector<uint32_t> rightOrder = topological_sort(mgr, mgr.systems.size(), [&](uint32_t idx) { return &mgr.systems[idx]; });
    apply_reorder(mgr.systems, rightOrder);
  }

  for (auto &[id, events] : mgr.eventIdToHandlers)
  {
    std::vector<uint32_t> rightOrder = topological_sort(mgr, events.size(), [&](uint32_t idx) { return &mgr.events[events[idx]]; });
    apply_reorder(events, rightOrder);
  }
}

bool try_registrate(ecs::EcsManager &mgr, ecs::Query &query, const ecs_details::Archetype *archetype)
{
  for (ComponentId componentId : query.requireComponents)
  {
    if (archetype->getComponentCollumnIndex(componentId) == -1)
    {
      return false;
    }
  }

  for (ComponentId componentId : query.excludeComponents)
  {
    if (archetype->getComponentCollumnIndex(componentId) != -1)
    {
      return false;
    }
  }

  ToComponentMap toComponentIndex;
  toComponentIndex.reserve(query.querySignature.size());
  std::vector<int> toTrackedComponentIndex;
  toTrackedComponentIndex.reserve(query.querySignature.size());


  for (const Query::ComponentAccessInfo &componentAccessInfo : query.querySignature)
  {
    int componentIndex = archetype->getComponentCollumnIndex(componentAccessInfo.componentId);
    if (componentIndex != -1)
    {
      toComponentIndex.push_back((std::vector<char *> *)&(archetype->collumns[componentIndex].chunks));

      if (componentAccessInfo.access == Query::ComponentAccess::READ_WRITE || componentAccessInfo.access == Query::ComponentAccess::READ_WRITE_OPTIONAL)
      {
        int trackedComponentIndex = archetype->getComponentTrackedCollumnIndex(componentAccessInfo.componentId);
        if (trackedComponentIndex != -1)
        {
          toTrackedComponentIndex.push_back(trackedComponentIndex);
        }
      }
    }
    else
    {
      auto it = mgr.singletons.find(get_type_id(componentAccessInfo.componentId));
      if (it != mgr.singletons.end())
      {
        toComponentIndex.push_back((std::vector<char *> *)it->second.data);
      }
      else if (!(componentAccessInfo.access == Query::ComponentAccess::READ_ONLY_OPTIONAL || componentAccessInfo.access == Query::ComponentAccess::READ_WRITE_OPTIONAL))
      {
        return false;
      }
      else
      {
        toComponentIndex.push_back(nullptr);
      }
    }
  }

  query.archetypesCache.emplace(archetype->archetypeId,
    ArchetypeRecord(
      (ecs_details::Archetype *)archetype,
      std::move(toComponentIndex),
      std::move(toTrackedComponentIndex)
    )
  );

  return true;
}


void register_query(EcsManager &mgr, Query &&query)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    try_registrate(mgr, query, archetype.get());
  }
  ECS_LOG_INFO_VERBOSE(mgr).log("Register query %s", query.uniqueName.c_str());
  mgr.queries[query.nameHash] = std::move(query);
}

void register_system(EcsManager &mgr, System &&system)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    try_registrate(mgr, system, archetype.get());
  }
  ECS_LOG_INFO_VERBOSE(mgr).log("Register system %s", system.uniqueName.c_str());
  mgr.systems.push_back(std::move(system));
}


void register_event(EcsManager &mgr, EventHandler &&event)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    try_registrate(mgr, event, archetype.get());
  }

  if (!event.trackedComponents.empty())
  {
    for (auto &[id, archetype] : mgr.archetypeMap)
    {
      ecs_details::try_registrate_track(mgr, event.trackedComponents, *archetype, event.nameHash);
    }
  }

  ECS_LOG_INFO_VERBOSE(mgr).log("Register event %s", event.uniqueName.c_str());
  for (EventId eventId : event.eventIds)
  {
    mgr.eventIdToHandlers[eventId].push_back(event.nameHash);
  }
  mgr.events[event.nameHash] = std::move(event);
}

void perform_system(const System &system)
{
  for (const auto &[archetypeId, archetypeRecord] : system.archetypesCache)
  {
    system.update_archetype(*archetypeRecord.archetype, archetypeRecord.toComponentIndex);
  }
}

void mark_dirty(ecs_details::Archetype &archetype, const std::vector<int> &to_tracked_component, uint32_t component_idx)
{
  for (int tracked_component_idx : to_tracked_component)
  {
    archetype.trackedCollumns[tracked_component_idx].mark_dirty(component_idx);
  }
}

void mark_dirty(ecs_details::Archetype &archetype, const std::vector<int> &to_tracked_component)
{
  for (int tracked_component_idx : to_tracked_component)
  {
    archetype.trackedCollumns[tracked_component_idx].mark_dirty();
  }
}

}