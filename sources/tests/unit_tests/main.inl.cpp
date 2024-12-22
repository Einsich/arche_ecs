#include <ecs/query_iteration.h>
template<typename Callable>
static void print_name_query(ecs::EcsManager &mgr, Callable &&query_function);

template<typename Callable>
static void print_name_by_eid_query(ecs::EcsManager &mgr, ecs::EntityId eid, Callable &&query_function);

#include "main.inl"
//Code-generator production

template<typename Callable>
static void print_name_query(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/unit_tests/main.inl:41[print_name_query]");
  const int N = 2;
  ecs::call_query<N, const std::string*, ecs::PrtWrapper<int>>(mgr, queryHash, std::move(query_function));
}

template<typename Callable>
static void print_name_by_eid_query(ecs::EcsManager &mgr, ecs::EntityId eid, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/unit_tests/main.inl:51[print_name_by_eid_query]");
  const int N = 2;
  ecs::call_query<N, const std::string*, ecs::PrtWrapper<int>>(mgr, eid, queryHash, std::move(query_function));
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

static void ecs_registration(ecs::EcsManager &mgr)
{
  {
    ecs::Query query;
    query.uniqueName = "sources/tests/unit_tests/main.inl:41[print_name_query]";
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
    query.uniqueName = "sources/tests/unit_tests/main.inl:51[print_name_by_eid_query]";
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
    ecs::System query;
    query.uniqueName = "sources/tests/unit_tests/main.inl:28[update]";
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
    query.uniqueName = "sources/tests/unit_tests/main.inl:34[print_name]";
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
}
static ecs::CodegenFileRegistration fileRegistration(&ecs_registration);
ECS_PULL_DEFINITION(variable_pull_main)
