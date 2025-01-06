#include "ecs/query.h"
#include "ecs/ecs_manager.h"

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
static std::vector<uint32_t> topological_sort(uint32_t nodeCount, const GetNode &get_node)
{
  Edges edge(nodeCount);
  std::vector<NodeState> used(nodeCount, NodeState::Black);
  std::vector<uint32_t> answer;
  answer.reserve(nodeCount);

  ska::flat_hash_map<std::string, std::vector<uint32_t>> nameMap;
  for (uint32_t i = 0; i < nodeCount; i++)
    nameMap[get_node(i)->name].push_back(i);

  for (uint32_t i = 0; i < nodeCount; i++)
  {
    const auto &query = *get_node(i);
    for (const std::string &before : query.before)
    {
      auto it = nameMap.find(before);
      if (it != nameMap.end())
      {
        for (uint32_t j : it->second)
        {
          if (i == j)
          {
            printf("system %s has itself in before\n", query.name.c_str());
            continue;
          }
          edge[i].push_back(j);
        }
      }
      else
      {
        printf("%s doesn't exist for before %s\n", before.c_str(), query.name.c_str());
      }
    }
    for (const std::string &after : query.after)
    {
      auto it = nameMap.find(after);
      if (it != nameMap.end())
      {
        for (uint32_t j : it->second)
        {
          if (i == j)
          {
            printf("system %s has itself in after\n", query.name.c_str());
            continue;
          }
          edge[j].push_back(i);
        }
      }
      else
      {
        printf("%s doesn't exist for after %s\n", after.c_str(), query.name.c_str());
      }
    }
  }
  const auto loger = [&](const std::vector<uint32_t> &nodes) {
    printf("cycle detected :\n");
    for (uint32_t i : nodes)
      printf("%s -> ", get_node(i)->name.c_str());
    printf("\n");
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
    std::vector<uint32_t> rightOrder = topological_sort(mgr.systems.size(), [&](uint32_t idx) { return &mgr.systems[idx]; });
    apply_reorder(mgr.systems, rightOrder);
  }

  for (auto &[id, events] : mgr.eventIdToHandlers)
  {
    std::vector<uint32_t> rightOrder = topological_sort(events.size(), [&](uint32_t idx) { return &mgr.events[events[idx]]; });
    apply_reorder(events, rightOrder);
  }
}

bool try_registrate(ecs::Query &query, const ecs_details::Archetype *archetype)
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
  for (const Query::ComponentAccessInfo &componentAccessInfo : query.querySignature)
  {
    int componentIndex = archetype->getComponentCollumnIndex(componentAccessInfo.componentId);
    if (componentIndex == -1 && !(componentAccessInfo.access == Query::ComponentAccess::READ_ONLY_OPTIONAL || componentAccessInfo.access == Query::ComponentAccess::READ_WRITE_OPTIONAL))
    {
      return false;
    }
    toComponentIndex.push_back(componentIndex >= 0 ? (std::vector<char *> *)&(archetype->collumns[componentIndex].chunks) : nullptr);
  }

  query.archetypesCache.insert({archetype->archetypeId, {(ecs_details::Archetype *)archetype, std::move(toComponentIndex)}});

  return true;
}

void register_query(EcsManager &mgr, Query &&query)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    try_registrate(query, archetype.get());
  }
  printf("[ECS] Register query %s\n", query.uniqueName.c_str());
  mgr.queries[query.nameHash] = std::move(query);
}

void register_system(EcsManager &mgr, System &&system)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    try_registrate(system, archetype.get());
  }
  printf("[ECS] Register system %s\n", system.uniqueName.c_str());
  mgr.systems.push_back(std::move(system));
}


void register_event(EcsManager &mgr, EventHandler &&event)
{
  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    try_registrate(event, archetype.get());
  }
  printf("[ECS] Register system %s\n", event.uniqueName.c_str());
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

}