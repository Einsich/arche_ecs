#include <ecs/codegen_helpers.h>
template<typename Callable>
static void print_name_query(ecs::EcsManager &mgr, Callable &&query_function);

template<typename Callable>
static void print_name_by_eid_query(ecs::EcsManager &mgr, ecs::EntityId eid, Callable &&query_function);

#include "main.inl"
//Code-generator production

template<typename Callable>
static void print_name_query(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/unit_tests/main.inl:33[print_name_query]");
  const int N = 2;
  ecs_details::query_iteration<N, ecs_details::Ptr<const std::string>, ecs_details::PrtWrapper<int>>(mgr, queryHash, std::move(query_function));
}

template<typename Callable>
static void print_name_by_eid_query(ecs::EcsManager &mgr, ecs::EntityId eid, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("sources/tests/unit_tests/main.inl:43[print_name_by_eid_query]");
  const int N = 2;
  ecs_details::query_invoke_for_entity<N, ecs_details::Ptr<const std::string>, ecs_details::PrtWrapper<int>>(mgr, eid, queryHash, std::move(query_function));
}

static void update_implementation(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  ecs_details::query_archetype_iteration<N, ecs_details::Ptr<ecs::EntityId>, ecs_details::Ptr<float3>, ecs_details::Ptr<const float3>>(archetype, to_archetype_component, update, std::make_index_sequence<N>());
}

static void print_name_implementation(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  ecs_details::query_archetype_iteration<N, ecs_details::Ptr<const std::string>, ecs_details::Ptr<float3>, ecs_details::PrtWrapper<int>>(archetype, to_archetype_component, print_name, std::make_index_sequence<N>());
}

static void update_with_singleton_implementation(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 2;
  ecs_details::query_archetype_iteration<N, ecs_details::Ptr<ecs::EntityId>, ecs_details::Ptr<SingletonComponent>>(archetype, to_archetype_component, update_with_singleton, std::make_index_sequence<N>());
}

static void on_appear_event_broadcast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 2;
  ecs_details::event_archetype_iteration<N, ecs_details::Ptr<const std::string>, ecs_details::PrtWrapper<const int>>(archetype, to_archetype_component, *(const ecs::OnAppear *)event_ptr, on_appear_event, std::make_index_sequence<N>());
}

static void on_appear_event_unicast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 2;
  ecs_details::event_invoke_for_entity<N, ecs_details::Ptr<const std::string>, ecs_details::PrtWrapper<const int>>(archetype, to_archetype_component, component_idx, *(const ecs::OnAppear *)event_ptr, on_appear_event, std::make_index_sequence<N>());
}

static void on_disappear_event_broadcast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 2;
  ecs_details::event_archetype_iteration<N, ecs_details::Ptr<const std::string>, ecs_details::PrtWrapper<const int>>(archetype, to_archetype_component, *(const ecs::OnDisappear *)event_ptr, on_disappear_event, std::make_index_sequence<N>());
}

static void on_disappear_event_unicast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 2;
  ecs_details::event_invoke_for_entity<N, ecs_details::Ptr<const std::string>, ecs_details::PrtWrapper<const int>>(archetype, to_archetype_component, component_idx, *(const ecs::OnDisappear *)event_ptr, on_disappear_event, std::make_index_sequence<N>());
}

static void appear_disapper_event_broadcast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 2;
  ecs_details::event_archetype_iteration<N, ecs_details::Ptr<const std::string>, ecs_details::PrtWrapper<const int>>(archetype, to_archetype_component, ecs::Event(event_id, event_ptr), appear_disapper_event, std::make_index_sequence<N>());
}

static void appear_disapper_event_unicast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 2;
  ecs_details::event_invoke_for_entity<N, ecs_details::Ptr<const std::string>, ecs_details::PrtWrapper<const int>>(archetype, to_archetype_component, component_idx, ecs::Event(event_id, event_ptr), appear_disapper_event, std::make_index_sequence<N>());
}

static void update_event_broadcast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 3;
  ecs_details::event_archetype_iteration<N, ecs_details::Ptr<ecs::EntityId>, ecs_details::Ptr<float3>, ecs_details::Ptr<const float3>>(archetype, to_archetype_component, *(const UpdateEvent *)event_ptr, update_event, std::make_index_sequence<N>());
}

static void update_event_unicast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 3;
  ecs_details::event_invoke_for_entity<N, ecs_details::Ptr<ecs::EntityId>, ecs_details::Ptr<float3>, ecs_details::Ptr<const float3>>(archetype, to_archetype_component, component_idx, *(const UpdateEvent *)event_ptr, update_event, std::make_index_sequence<N>());
}

static void heavy_event_broadcast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 1;
  ecs_details::event_archetype_iteration<N, ecs_details::Ptr<ecs::EntityId>>(archetype, to_archetype_component, *(const HeavyEvent *)event_ptr, heavy_event, std::make_index_sequence<N>());
}

static void heavy_event_unicast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 1;
  ecs_details::event_invoke_for_entity<N, ecs_details::Ptr<ecs::EntityId>>(archetype, to_archetype_component, component_idx, *(const HeavyEvent *)event_ptr, heavy_event, std::make_index_sequence<N>());
}

static void multi_event_broadcast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 1;
  ecs_details::event_archetype_iteration<N, ecs_details::Ptr<ecs::EntityId>>(archetype, to_archetype_component, ecs::Event(event_id, event_ptr), multi_event, std::make_index_sequence<N>());
}

static void multi_event_unicast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)
{
  ECS_UNUSED(event_id);
  const int N = 1;
  ecs_details::event_invoke_for_entity<N, ecs_details::Ptr<ecs::EntityId>>(archetype, to_archetype_component, component_idx, ecs::Event(event_id, event_ptr), multi_event, std::make_index_sequence<N>());
}

static void ecs_registration(ecs::EcsManager &mgr)
{
  ECS_UNUSED(mgr);
  {
    ecs::Query query;
    query.name = "print_name_query";
    query.uniqueName = "sources/tests/unit_tests/main.inl:33[print_name_query]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<std::string>::typeId, "name"), ecs::Query::ComponentAccess::READ_ONLY},
      {ecs::get_component_id(ecs::TypeInfo<int>::typeId, "health"), ecs::Query::ComponentAccess::READ_WRITE_OPTIONAL}
    };
    ecs::register_query(mgr, std::move(query));
  }
  {
    ecs::Query query;
    query.name = "print_name_by_eid_query";
    query.uniqueName = "sources/tests/unit_tests/main.inl:43[print_name_by_eid_query]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<std::string>::typeId, "name"), ecs::Query::ComponentAccess::READ_ONLY},
      {ecs::get_component_id(ecs::TypeInfo<int>::typeId, "health"), ecs::Query::ComponentAccess::READ_WRITE_OPTIONAL}
    };
    ecs::register_query(mgr, std::move(query));
  }
  {
    ecs::System query;
    query.name = "update";
    query.uniqueName = "sources/tests/unit_tests/main.inl:19[update]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<ecs::EntityId>::typeId, "eid"), ecs::Query::ComponentAccess::READ_COPY},
      {ecs::get_component_id(ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_component_id(ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_ONLY}
    };
    query.requireComponents =
    {
      ecs::get_component_id(ecs::TypeInfo<std::string>::typeId, "name")
    };
    query.excludeComponents =
    {
      ecs::get_component_id(ecs::TypeInfo<int>::typeId, "health")
    };
    query.update_archetype = update_implementation;
    ecs::register_system(mgr, std::move(query));
  }
  {
    ecs::System query;
    query.name = "print_name";
    query.uniqueName = "sources/tests/unit_tests/main.inl:26[print_name]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<std::string>::typeId, "name"), ecs::Query::ComponentAccess::READ_ONLY},
      {ecs::get_component_id(ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_COPY},
      {ecs::get_component_id(ecs::TypeInfo<int>::typeId, "health"), ecs::Query::ComponentAccess::READ_WRITE_OPTIONAL}
    };
    query.update_archetype = print_name_implementation;
    ecs::register_system(mgr, std::move(query));
  }
  {
    ecs::System query;
    query.name = "update_with_singleton";
    query.uniqueName = "sources/tests/unit_tests/main.inl:111[update_with_singleton]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<ecs::EntityId>::typeId, "eid"), ecs::Query::ComponentAccess::READ_COPY},
      {ecs::get_component_id(ecs::TypeInfo<SingletonComponent>::typeId, "singleton"), ecs::Query::ComponentAccess::READ_WRITE}
    };
    query.requireComponents =
    {
      ecs::get_component_id(ecs::TypeInfo<std::string>::typeId, "name")
    };
    query.excludeComponents =
    {
      ecs::get_component_id(ecs::TypeInfo<int>::typeId, "health")
    };
    query.update_archetype = update_with_singleton_implementation;
    ecs::register_system(mgr, std::move(query));
  }
  {
    ecs::EventHandler query;
    query.name = "on_appear_event";
    query.uniqueName = "sources/tests/unit_tests/main.inl:50[on_appear_event]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<std::string>::typeId, "name"), ecs::Query::ComponentAccess::READ_ONLY},
      {ecs::get_component_id(ecs::TypeInfo<int>::typeId, "health"), ecs::Query::ComponentAccess::READ_ONLY_OPTIONAL}
    };
    query.before = {"appear_disapper_event", };
    query.broadcastEvent = on_appear_event_broadcast_event;
    query.unicastEvent = on_appear_event_unicast_event;
    query.eventIds = {ecs::EventInfo<ecs::OnAppear>::eventId};
    ecs::register_event(mgr, std::move(query));
  }
  {
    ecs::EventHandler query;
    query.name = "on_disappear_event";
    query.uniqueName = "sources/tests/unit_tests/main.inl:55[on_disappear_event]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<std::string>::typeId, "name"), ecs::Query::ComponentAccess::READ_ONLY},
      {ecs::get_component_id(ecs::TypeInfo<int>::typeId, "health"), ecs::Query::ComponentAccess::READ_ONLY_OPTIONAL}
    };
    query.before = {"appear_disapper_event", };
    query.broadcastEvent = on_disappear_event_broadcast_event;
    query.unicastEvent = on_disappear_event_unicast_event;
    query.eventIds = {ecs::EventInfo<ecs::OnDisappear>::eventId};
    ecs::register_event(mgr, std::move(query));
  }
  {
    ecs::EventHandler query;
    query.name = "appear_disapper_event";
    query.uniqueName = "sources/tests/unit_tests/main.inl:60[appear_disapper_event]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<std::string>::typeId, "name"), ecs::Query::ComponentAccess::READ_ONLY},
      {ecs::get_component_id(ecs::TypeInfo<int>::typeId, "health"), ecs::Query::ComponentAccess::READ_ONLY_OPTIONAL}
    };
    query.after = {"on_appear_event", "on_disappear_event", };
    query.broadcastEvent = appear_disapper_event_broadcast_event;
    query.unicastEvent = appear_disapper_event_unicast_event;
    query.eventIds = {ecs::EventInfo<ecs::OnAppear>::eventId, ecs::EventInfo<ecs::OnDisappear>::eventId, };
    ecs::register_event(mgr, std::move(query));
  }
  {
    ecs::EventHandler query;
    query.name = "update_event";
    query.uniqueName = "sources/tests/unit_tests/main.inl:83[update_event]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<ecs::EntityId>::typeId, "eid"), ecs::Query::ComponentAccess::READ_COPY},
      {ecs::get_component_id(ecs::TypeInfo<float3>::typeId, "position"), ecs::Query::ComponentAccess::READ_WRITE},
      {ecs::get_component_id(ecs::TypeInfo<float3>::typeId, "velocity"), ecs::Query::ComponentAccess::READ_ONLY}
    };
    query.broadcastEvent = update_event_broadcast_event;
    query.unicastEvent = update_event_unicast_event;
    query.eventIds = {ecs::EventInfo<UpdateEvent>::eventId};
    ecs::register_event(mgr, std::move(query));
  }
  {
    ecs::EventHandler query;
    query.name = "heavy_event";
    query.uniqueName = "sources/tests/unit_tests/main.inl:88[heavy_event]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<ecs::EntityId>::typeId, "eid"), ecs::Query::ComponentAccess::READ_COPY}
    };
    query.broadcastEvent = heavy_event_broadcast_event;
    query.unicastEvent = heavy_event_unicast_event;
    query.eventIds = {ecs::EventInfo<HeavyEvent>::eventId};
    ecs::register_event(mgr, std::move(query));
  }
  {
    ecs::EventHandler query;
    query.name = "multi_event";
    query.uniqueName = "sources/tests/unit_tests/main.inl:94[multi_event]";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature =
    {
      {ecs::get_component_id(ecs::TypeInfo<ecs::EntityId>::typeId, "eid"), ecs::Query::ComponentAccess::READ_COPY}
    };
    query.broadcastEvent = multi_event_broadcast_event;
    query.unicastEvent = multi_event_unicast_event;
    query.eventIds = {ecs::EventInfo<UpdateEvent>::eventId, ecs::EventInfo<HeavyEvent>::eventId, };
    ecs::register_event(mgr, std::move(query));
  }
}
static ecs_details::CodegenFileRegistration fileRegistration(&ecs_registration);
ECS_PULL_DEFINITION(variable_pull_main)
