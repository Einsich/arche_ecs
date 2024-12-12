#include "systems.inl"
#include <ecs/query_iteration.h>
//Code-generator production

template<typename Callable>
static void print_name_query(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/ecs/systems.inl:26[print_name_query]");
  ecs::call_query(mgr, queryHash, [&](ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
  {
    const int N = 2;
    ecs::templated_archetype_iterate<N, const std::string*, ecs::PrtWrapper<int>>(archetype, to_archetype_component, std::move(query_function), std::make_index_sequence<N>());
  });
}

template<typename Callable>
static void query_perf_test1(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/ecs/systems.inl:60[query_perf_test1]");
  ecs::call_query(mgr, queryHash, [&](ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
  {
    const int N = 1;
    ecs::templated_archetype_iterate<N, float3*>(archetype, to_archetype_component, std::move(query_function), std::make_index_sequence<N>());
  });
}

template<typename Callable>
static void query_perf_test2(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/ecs/systems.inl:68[query_perf_test2]");
  ecs::call_query(mgr, queryHash, [&](ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
  {
    const int N = 2;
    ecs::templated_archetype_iterate<N, float3*, const float3*>(archetype, to_archetype_component, std::move(query_function), std::make_index_sequence<N>());
  });
}

template<typename Callable>
static void query_perf_test3(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/ecs/systems.inl:76[query_perf_test3]");
  ecs::call_query(mgr, queryHash, [&](ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
  {
    const int N = 3;
    ecs::templated_archetype_iterate<N, float3*, float3*, const float3*>(archetype, to_archetype_component, std::move(query_function), std::make_index_sequence<N>());
  });
}

static void update_implementation(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  ecs::templated_archetype_iterate<N, ecs::EntityId*, float3*, const float3*>(archetype, to_archetype_component, update, std::make_index_sequence<N>());
}

static void print_name_implementation(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  ecs::templated_archetype_iterate<N, const std::string*, float3*, ecs::PrtWrapper<int>>(archetype, to_archetype_component, print_name, std::make_index_sequence<N>());
}

static void system_perf_test1_implementation(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 1;
  ecs::templated_archetype_iterate<N, float3 *>(archetype, to_archetype_component, system_perf_test1, std::make_index_sequence<N>());
}

static void system_perf_test2_implementation(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 2;
  ecs::templated_archetype_iterate<N, float3*, const float3*>(archetype, to_archetype_component, system_perf_test2, std::make_index_sequence<N>());
}

static void system_perf_test3_implementation(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  ecs::templated_archetype_iterate<N, float3*, float3*, const float3*>(archetype, to_archetype_component, system_perf_test3, std::make_index_sequence<N>());
}

static void ecs_registration(ecs::EcsManager &mgr)
{
  {
    ecs::Query query;
    query.uniqueName = "sources/ecs/systems.inl:26[print_name_query]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<std::string>::typeId, "name"), ecs::Query::ComponentAccess::READ_ONLY},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<int>::typeId, "health"), ecs::Query::ComponentAccess::READ_WRITE_OPTIONAL}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    ecs::register_query(mgr, std::move(query));
  }
  {
    ecs::Query query;
    query.uniqueName = "sources/ecs/systems.inl:60[query_perf_test1]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    ecs::register_query(mgr, std::move(query));
  }
  {
    ecs::Query query;
    query.uniqueName = "sources/ecs/systems.inl:68[query_perf_test2]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_ONLY}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    ecs::register_query(mgr, std::move(query));
  }
  {
    ecs::Query query;
    query.uniqueName = "sources/ecs/systems.inl:76[query_perf_test3]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "acceleration"), ecs::Query::ComponentAccess::READ_ONLY}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    ecs::register_query(mgr, std::move(query));
  }
  {
    ecs::System query;
    query.uniqueName = "sources/ecs/systems.inl:10[update]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<ecs::EntityId>::typeId, "eid"), ecs::Query::ComponentAccess::READ_COPY},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_ONLY}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    query.update_archetype = update_implementation;
    ecs::register_system(mgr, std::move(query));
  }
  {
    ecs::System query;
    query.uniqueName = "sources/ecs/systems.inl:16[print_name]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<std::string>::typeId, "name"), ecs::Query::ComponentAccess::READ_ONLY},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_COPY},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<int>::typeId, "health"), ecs::Query::ComponentAccess::READ_WRITE_OPTIONAL}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    query.update_archetype = print_name_implementation;
    ecs::register_system(mgr, std::move(query));
  }
  {
    ecs::System query;
    query.uniqueName = "sources/ecs/systems.inl:34[system_perf_test1]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    query.update_archetype = system_perf_test1_implementation;
    ecs::register_system(mgr, std::move(query));
  }
  {
    ecs::System query;
    query.uniqueName = "sources/ecs/systems.inl:40[system_perf_test2]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_ONLY}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    query.update_archetype = system_perf_test2_implementation;
    ecs::register_system(mgr, std::move(query));
  }
  {
    ecs::System query;
    query.uniqueName = "sources/ecs/systems.inl:45[system_perf_test3]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "acceleration"), ecs::Query::ComponentAccess::READ_ONLY}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    query.update_archetype = system_perf_test3_implementation;
    ecs::register_system(mgr, std::move(query));
  }
}
static ecs::CodegenFileRegistration fileRegistration(&ecs_registration);
ECS_PULL_DEFINITION(variable_pull_systems)
