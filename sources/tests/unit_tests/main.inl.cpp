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

static void update_event_broadcast_event(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 3;
  ecs::templated_event_archetype_iterate<N, ecs::EntityId*, float3*, const float3*>(archetype, to_archetype_component, *(const UpdateEvent *)event_ptr, update_event, std::make_index_sequence<N>());
}

static void update_event_unicast_event(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 3;
  ecs::templated_archetype_event_one_entity<N, ecs::EntityId*, float3*, const float3*>(archetype, to_archetype_component, component_idx, *(const UpdateEvent *)event_ptr, update_event, std::make_index_sequence<N>());
}

static void heavy_event_broadcast_event(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 1;
  ecs::templated_event_archetype_iterate<N, ecs::EntityId*>(archetype, to_archetype_component, *(const HeavyEvent *)event_ptr, heavy_event, std::make_index_sequence<N>());
}

static void heavy_event_unicast_event(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 1;
  ecs::templated_archetype_event_one_entity<N, ecs::EntityId*>(archetype, to_archetype_component, component_idx, *(const HeavyEvent *)event_ptr, heavy_event, std::make_index_sequence<N>());
}

static void multi_event_broadcast_event(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 1;
  ecs::templated_event_archetype_iterate<N, ecs::EntityId*>(archetype, to_archetype_component, ecs::Event(event_id, event_ptr), multi_event, std::make_index_sequence<N>());
}

static void multi_event_unicast_event(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 1;
  ecs::templated_archetype_event_one_entity<N, ecs::EntityId*>(archetype, to_archetype_component, component_idx, ecs::Event(event_id, event_ptr), multi_event, std::make_index_sequence<N>());
}

static void ecs_registration(ecs::EcsManager &mgr)
{
  ECS_UNUSED(mgr);
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
  {
    ecs::EventHandler query;
    query.uniqueName = "sources/tests/unit_tests/main.inl:72[update_event]";
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
    query.broadcastEvent = update_event_broadcast_event;
    query.unicastEvent = update_event_unicast_event;
    query.eventIds = {ecs::EventInfo<UpdateEvent>::eventId};
    ecs::register_event(mgr, std::move(query));
  }
  {
    ecs::EventHandler query;
    query.uniqueName = "sources/tests/unit_tests/main.inl:77[heavy_event]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<ecs::EntityId>::typeId, "eid"), ecs::Query::ComponentAccess::READ_COPY}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    query.broadcastEvent = heavy_event_broadcast_event;
    query.unicastEvent = heavy_event_unicast_event;
    query.eventIds = {ecs::EventInfo<HeavyEvent>::eventId};
    ecs::register_event(mgr, std::move(query));
  }
  {
    ecs::EventHandler query;
    query.uniqueName = "sources/tests/unit_tests/main.inl:83[multi_event]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<ecs::EntityId>::typeId, "eid"), ecs::Query::ComponentAccess::READ_COPY}
    };
    query.requireComponents =
    {
    };
    query.excludeComponents =
    {
    };
    query.broadcastEvent = multi_event_broadcast_event;
    query.unicastEvent = multi_event_unicast_event;
    query.eventIds = {ecs::EventInfo<UpdateEvent>::eventId, ecs::EventInfo<HeavyEvent>::eventId, };
    ecs::register_event(mgr, std::move(query));
  }
}
static ecs::CodegenFileRegistration fileRegistration(&ecs_registration);
ECS_PULL_DEFINITION(variable_pull_main)
