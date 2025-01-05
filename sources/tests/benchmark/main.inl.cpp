#include <ecs/codegen_helpers.h>
template<typename Callable>
static void query_perf_test1(ecs::EcsManager &mgr, Callable &&query_function);

template<typename Callable>
static void query_perf_test2(ecs::EcsManager &mgr, Callable &&query_function);

template<typename Callable>
static void query_perf_test3(ecs::EcsManager &mgr, Callable &&query_function);

#include "main.inl"
//Code-generator production

template<typename Callable>
static void query_perf_test1(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/benchmark/main.inl:416[query_perf_test1]");
  const int N = 1;
  ecs_details::query_iteration<N, float3*>(mgr, queryHash, std::move(query_function));
}

template<typename Callable>
static void query_perf_test2(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/benchmark/main.inl:424[query_perf_test2]");
  const int N = 2;
  ecs_details::query_iteration<N, float3*, const float3*>(mgr, queryHash, std::move(query_function));
}

template<typename Callable>
static void query_perf_test3(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/benchmark/main.inl:432[query_perf_test3]");
  const int N = 3;
  ecs_details::query_iteration<N, float3*, float3*, const float3*>(mgr, queryHash, std::move(query_function));
}

static void system_perf_test1_implementation(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 1;
  ecs_details::query_archetype_iteration<N, float3*>(archetype, to_archetype_component, system_perf_test1, std::make_index_sequence<N>());
}

static void system_perf_test2_implementation(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 2;
  ecs_details::query_archetype_iteration<N, float3*, const float3*>(archetype, to_archetype_component, system_perf_test2, std::make_index_sequence<N>());
}

static void system_perf_test3_implementation(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  ecs_details::query_archetype_iteration<N, float3*, float3*, const float3*>(archetype, to_archetype_component, system_perf_test3, std::make_index_sequence<N>());
}

static void ecs_registration(ecs::EcsManager &mgr)
{
  ECS_UNUSED(mgr);
  {
    ecs::Query query;
    query.uniqueName = "sources/tests/benchmark/main.inl:416[query_perf_test1]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE}
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
    query.uniqueName = "sources/tests/benchmark/main.inl:424[query_perf_test2]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_ONLY}
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
    query.uniqueName = "sources/tests/benchmark/main.inl:432[query_perf_test3]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "acceleration"), ecs::Query::ComponentAccess::READ_ONLY}
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
    query.uniqueName = "sources/tests/benchmark/main.inl:397[system_perf_test1]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE}
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
    query.uniqueName = "sources/tests/benchmark/main.inl:403[system_perf_test2]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_ONLY}
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
    query.uniqueName = "sources/tests/benchmark/main.inl:408[system_perf_test3]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "acceleration"), ecs::Query::ComponentAccess::READ_ONLY}
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
static ecs_details::CodegenFileRegistration fileRegistration(&ecs_registration);
ECS_PULL_DEFINITION(variable_pull_main)
