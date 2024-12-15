#include "main.inl"
#include <ecs/query_iteration.h>
//Code-generator production

template<typename Callable>
static void query_perf_test1(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/benchmark/main.inl:387[query_perf_test1]");
  ecs::call_query(mgr, queryHash, [&](ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
  {
    const int N = 1;
    ecs::templated_archetype_iterate<N, float3*>(archetype, to_archetype_component, std::move(query_function), std::make_index_sequence<N>());
  });
}

template<typename Callable>
static void query_perf_test2(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/benchmark/main.inl:395[query_perf_test2]");
  ecs::call_query(mgr, queryHash, [&](ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
  {
    const int N = 2;
    ecs::templated_archetype_iterate<N, float3*, const float3*>(archetype, to_archetype_component, std::move(query_function), std::make_index_sequence<N>());
  });
}

template<typename Callable>
static void query_perf_test3(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/benchmark/main.inl:403[query_perf_test3]");
  ecs::call_query(mgr, queryHash, [&](ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
  {
    const int N = 3;
    ecs::templated_archetype_iterate<N, float3*, float3*, const float3*>(archetype, to_archetype_component, std::move(query_function), std::make_index_sequence<N>());
  });
}

static void system_perf_test1_implementation(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 1;
  ecs::templated_archetype_iterate<N, float3*>(archetype, to_archetype_component, system_perf_test1, std::make_index_sequence<N>());
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
    query.uniqueName = "sources/tests/benchmark/main.inl:387[query_perf_test1]";
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
    query.uniqueName = "sources/tests/benchmark/main.inl:395[query_perf_test2]";
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
    query.uniqueName = "sources/tests/benchmark/main.inl:403[query_perf_test3]";
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
    query.uniqueName = "sources/tests/benchmark/main.inl:361[system_perf_test1]";
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
    query.uniqueName = "sources/tests/benchmark/main.inl:367[system_perf_test2]";
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
    query.uniqueName = "sources/tests/benchmark/main.inl:372[system_perf_test3]";
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
ECS_PULL_DEFINITION(variable_pull_main)