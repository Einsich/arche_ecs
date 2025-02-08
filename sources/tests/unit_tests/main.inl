#include "ecs/ecs.h"
#include "ecs/ecs_manager.h"
#include <assert.h>
#include "math_helper.h"
#include "timer.h"
#include "logger.h"

void query_test(ecs::EcsManager &mgr);

ECS_TYPE_DECLARATION(int)
ECS_TYPE_DECLARATION(float3)
ECS_TYPE_DECLARATION_ALIAS(std::string, "string")


ECS_TYPE_REGISTRATION(int)
ECS_TYPE_REGISTRATION(float3)
ECS_TYPE_REGISTRATION(std::string)

ECS_SYSTEM(require=std::string name; require_not=int health; stage=editor_act)
editor_update(ecs::EntityId eid, float3 &position, const float3 &velocity)
{
  printf("editor_update [%d/%d] (%f %f %f), (%f %f %f)\n", eid.entityIndex, eid.generation, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
}

ECS_SYSTEM(require=std::string name; require_not=int health)
update(ecs::EntityId eid, float3 &position, const float3 &velocity)
{
  printf("update [%d/%d] (%f %f %f), (%f %f %f)\n", eid.entityIndex, eid.generation, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
}


ECS_SYSTEM() print_name(const std::string &name, float3 position, int *health)
{
  printf("print_name [%s] (%f %f %f), %d\n", name.c_str(), position.x, position.y, position.z, health ? *health : -1);
}

void query_test(ecs::EcsManager &mgr)
{
  ECS_QUERY() print_name_query(mgr, [](const std::string &name, int *health)
  {
    printf("print_name [%s] %d\n", name.c_str(), health ? *health : -1);
  });
}

void query_by_eid_test(ecs::EcsManager &mgr, const std::vector<ecs::EntityId> &eids)
{
  for (ecs::EntityId eid : eids)
  {
    ECS_QUERY() print_name_by_eid_query(mgr, eid, [](const std::string &name, int *health)
    {
      printf("print_name_by_eid [%s] %d\n", name.c_str(), health ? *health : -1);
    });
  }
}

ECS_EVENT(before=appear_disapper_event) on_appear_event(const ecs::OnAppear &, const std::string &name, const int *health)
{
  printf("on_appear_event [%s] %d\n", name.c_str(), health ? *health : -1);
}

ECS_EVENT(before=appear_disapper_event) on_disappear_event(const ecs::OnDisappear &, const std::string &name, const int *health)
{
  printf("on_disappear_event [%s] %d\n", name.c_str(), health ? *health : -1);
}

ECS_EVENT(on_event=ecs::OnAppear, ecs::OnDisappear; after=on_appear_event, on_disappear_event)
appear_disapper_event(const ecs::Event &event, const std::string &name, const int *health)
{
  if (const ecs::OnAppear *updateEvent = event.cast<ecs::OnAppear>())
    printf("appear_disapper_event OnAppear [%s] %d\n", name.c_str(), health ? *health : -1);
  else if (const ecs::OnDisappear *heavyEvent = event.cast<ecs::OnDisappear>())
    printf("appear_disapper_event OnDisappear [%s] %d\n", name.c_str(), health ? *health : -1);
}

ECS_EVENT(track=int health)
health_changed(const ecs::Event &, const std::string &name, int health)
{
  printf("health_changed [%s] %d\n", name.c_str(), health);
}
struct UpdateEvent
{

};

ECS_EVENT_DECLARATION(UpdateEvent)

struct HeavyEvent
{
  std::vector<int> data;
};

ECS_EVENT_DECLARATION(HeavyEvent)

ECS_EVENT() update_event(const UpdateEvent &, ecs::EntityId eid, float3 &position, const float3 &velocity)
{
  printf("update_event [%d/%d] (%f %f %f), (%f %f %f)\n", eid.entityIndex, eid.generation, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
}

ECS_EVENT() heavy_event(const HeavyEvent &event, ecs::EntityId eid)
{
  ECS_UNUSED(event);
  assert(event.data.size() == 100);
  printf("heavy_event [%d/%d]\n", eid.entityIndex, eid.generation);
}

ECS_EVENT(on_event=UpdateEvent, HeavyEvent)
multi_event(const ecs::Event &event, ecs::EntityId eid)
{
  if (const UpdateEvent *updateEvent = event.cast<UpdateEvent>())
    printf("multi_event UpdateEvent [%d/%d]\n", eid.entityIndex, eid.generation);
  else if (const HeavyEvent *heavyEvent = event.cast<HeavyEvent>())
    printf("multi_event HeavyEvent [%d/%d]\n", eid.entityIndex, eid.generation);
}

struct SingletonComponent
{
  int value;
};

ECS_SINGLETON_TYPE_DECLARATION(SingletonComponent)
ECS_TYPE_REGISTRATION(SingletonComponent)

ECS_SYSTEM(require=std::string name; require_not=int health)
update_with_singleton(ecs::EntityId eid, SingletonComponent &singleton)
{
  printf("update_with_singleton [%d/%d] SingletonComponent.value = %d\n", eid.entityIndex, eid.generation, singleton.value++);
}

int main()
{
  const bool EntityContainerTest = true;
  if (EntityContainerTest)
  {
    ecs_details::EntityContainer entityContainer;
    ecs::EntityId entityId = entityContainer.allocate_entity(ecs_details::EntityState::Alive);
    assert(entityContainer.is_alive(entityId));
    entityContainer.destroy_entity(entityId);
    assert(!entityContainer.is_alive(entityId));
    assert(!entityContainer.is_alive(ecs::EntityId()));

    ecs::EntityId entityId1 = entityContainer.allocate_entity(ecs_details::EntityState::Alive);
    assert(entityContainer.is_alive(entityId1));
    assert(entityId != entityId1);
    ECS_UNUSED(entityId);
    ECS_UNUSED(entityId1);
  }
  ecs::EcsManager mgr;

  mgr.logger = std::unique_ptr<Logger>(new Logger());

  ecs::register_all_type_declarations(mgr);

  ecs::register_all_codegen_files(mgr);

  ecs::sort_systems(mgr);

  ecs::init_singletons(mgr);

  for (const auto &[id, type] : mgr.typeMap)
  {
    printf("[ECS] type: %s, typeId: %x\n", type.typeName.c_str(), type.typeId);
  }

  ecs::ComponentId positionId = ecs::get_or_add_component<float3>(mgr, "position");
  ecs::ComponentId velocityId = ecs::get_or_add_component<float3>(mgr, "velocity");
  ecs::ComponentId healthId = ecs::get_or_add_component<int>(mgr, "health");
  ecs::ComponentId nameId = ecs::get_or_add_component<std::string>(mgr, "name");

  for (const auto &[id, component] : mgr.componentMap)
  {
    printf("[ECS] component: %s, componentId: %llx, typeId: %x\n", component->name.c_str(), component->componentId, component->typeId);
  }

  ecs::TemplateId template1 = template_registration(mgr, "point",
    {mgr, {
      {positionId, float3{}},
      {velocityId, float3{}}
    }}, ecs::ArchetypeChunkSize::Dozens);

  ecs::TemplateId template2 = template_registration(mgr, "named_point",
    {mgr, {
      {velocityId, float3{}},
      {positionId, float3{}},
      {nameId, std::string{}}
    }}, ecs::ArchetypeChunkSize::Dozens);

  ecs::TemplateInit templateInit;
  templateInit.name = "named_alive_point";
  templateInit.chunkSizePower = ecs::ArchetypeChunkSize::Dozens;
  templateInit.args = {mgr, {
    {velocityId, float3{}},
    {positionId, float3{}},
    {nameId, std::string{}},
    {healthId, 0}
  }};
  templateInit.trackedComponents = {"health"};
  ecs::TemplateId template3 = template_registration(mgr, std::move(templateInit));

  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    printf("[ECS] archetype: %x, components: %zu\n", archetype->archetypeId, archetype->collumns.size());
  }


  const float3 positionValue = float3{1, 2, 3};
  const float3 velocityValue = float3{4, 5, 6};
  const float3 newPositionValue = float3{3, 2, 1};
  ECS_UNUSED(positionValue);
  ECS_UNUSED(velocityValue);
  ECS_UNUSED(newPositionValue);

  {
    ecs::InitializerList args(mgr);
    args.push_back(ecs::ComponentInit{"position", float3{1, 2, 3}});
    args.push_back(ecs::ComponentInit{"velocity", float3{4, 5, 6}});
    ecs::EntityId eid = ecs::create_entity_sync(mgr, template1, std::move(args));
    float3 newPositionValueRW = float3{3, 2, 1};
    ECS_UNUSED(eid);
    ECS_UNUSED(newPositionValueRW);
    assert(*ecs::get_component<float3>(mgr, eid, "position") == positionValue);
    assert(*ecs::get_rw_component<float3>(mgr, eid, "position") == positionValue);
    assert(ecs::get_component<int>(mgr, eid, "position") == nullptr);
    assert(ecs::get_component<float3>(mgr, eid, "positions") == nullptr);
    assert(ecs::set_component<float3>(mgr, eid, "position", newPositionValue) == true);
    assert(ecs::set_component<float3>(mgr, eid, "position", newPositionValueRW) == true);
    assert(ecs::set_component<float3>(mgr, eid, "position", float3{newPositionValue}) == true);
    assert(*ecs::get_component<float3>(mgr, eid, "position") == newPositionValue);
  }


  {
    ecs::InitializerList args(mgr);
    args.push_back(ecs::ComponentInit{"position", float3{1, 2, 3}});
    args.push_back(ecs::ComponentInit{"velocity", float3{4, 5, 6}});


    ecs::EntityId eid = ecs::create_entity(mgr, template1, std::move(args));
    ECS_UNUSED(eid);
    assert(mgr.entityContainer.is_alive(eid));
    assert(!mgr.entityContainer.can_access(eid));
    assert(ecs::get_component<float3>(mgr, eid, "position") == nullptr);
    assert(ecs::get_component<float3>(mgr, eid, "velocity") == nullptr);

    ecs::perform_delayed_entities_creation(mgr);

    assert(mgr.entityContainer.is_alive(eid));
    assert(mgr.entityContainer.can_access(eid));
    assert(*ecs::get_component<float3>(mgr, eid, "position") == positionValue);
    assert(*ecs::get_component<float3>(mgr, eid, "velocity") == velocityValue);
  }
  {
    std::vector<float3> positions = {float3{1, 2, 3}};
    std::vector<float3> velocities = {float3{4, 5, 6}};
    std::vector<ecs::EntityId> eids = ecs::create_entities(mgr, template1,
    {{
      {"position", std::move(positions)},
      {"velocity", std::move(velocities)}
    }});
    ecs::EntityId eid = eids[0];
    ECS_UNUSED(eid);

    assert(mgr.entityContainer.is_alive(eid));
    assert(!mgr.entityContainer.can_access(eid));
    assert(ecs::get_component<float3>(mgr, eid, "position") == nullptr);
    assert(ecs::get_component<float3>(mgr, eid, "velocity") == nullptr);

    ecs::perform_delayed_entities_creation(mgr);

    assert(mgr.entityContainer.is_alive(eid));
    assert(mgr.entityContainer.can_access(eid));
    assert(*ecs::get_component<float3>(mgr, eid, "position") == positionValue);
    assert(*ecs::get_component<float3>(mgr, eid, "velocity") == velocityValue);
  }
  {
    ecs::EntityId eid = ecs::create_entity(mgr, template1);
    ECS_UNUSED(eid);
    ecs::destroy_entity(mgr, eid);
    assert(!mgr.entityContainer.is_alive(eid));
    assert(!mgr.entityContainer.can_access(eid));
    assert(ecs::get_component<float3>(mgr, eid, "position") == nullptr);
    assert(ecs::get_component<float3>(mgr, eid, "velocity") == nullptr);
    ecs::perform_delayed_entities_creation(mgr);
    assert(!mgr.entityContainer.is_alive(eid));
    assert(!mgr.entityContainer.can_access(eid));
    assert(ecs::get_component<float3>(mgr, eid, "position") == nullptr);
    assert(ecs::get_component<float3>(mgr, eid, "velocity") == nullptr);
  }


  ecs::create_entity_sync(mgr,
    template3,
    ecs::InitializerList{mgr,
      {
        ecs::ComponentInit{"position", float3{7, 8, 9}},
        ecs::ComponentInit{"velocity", float3{10, 11, 12}},
        ecs::ComponentInit{"health", 100},
        ecs::ComponentInit{"name", std::string("entity1")}
      }
    }
  );

  ecs::TemplateId movableTemplate = template_registration(mgr, "movable",
    {mgr, {
      {positionId, float3{0, 0, 0}},
      {velocityId, float3{0, 0, 0}}
    }}
  );
  ecs::TemplateId brickTemplate = template_registration(mgr, "brick", movableTemplate,
    {mgr, {
      {positionId, float3{1, 0, 0}},
      {healthId, 10},
      {nameId, std::string("brick")}
    }}
  );
  ecs::TemplateId humanTemplate = template_registration(mgr, "human", movableTemplate,
    {mgr, {
      {positionId, float3{2, 0, 0}},
      // {healthId, 50},
      {nameId, std::string("human")}
    }}
  );
  ecs::TemplateId bigBrickTemplate = template_registration(mgr, "bigbrick", brickTemplate,
    {mgr, {
      {positionId, float3{3, 0, 0}},
      {healthId, 100}
    }}
  );

  ecs::create_entity_sync(mgr, movableTemplate);
  ecs::create_entity_sync(mgr, brickTemplate);
  ecs::create_entity_sync(mgr, humanTemplate);
  ecs::create_entity_sync(mgr, bigBrickTemplate);


  // if (false)
  {
    Timer timer("create_entity_sync"); // (0.066700 ms)
    for (int i = 0; i < 5; i++)
    {
      float f = i;
      ecs::EntityId eid = ecs::create_entity_sync(mgr,
        template2,
        {mgr, {
          {"position", float3{f, f, f}},
          // {velocityId, float3{f, f, f}},
          {"name", std::string("very_long_node_name") + std::to_string(i)}
        }}
      );
      ECS_UNUSED(eid);
    }
  }

  // if (false)
  {
    Timer timer("create_entities"); // (0.046000 ms)
    std::vector<float3> positions;
    std::vector<std::string> names;
    positions.reserve(5);
    names.reserve(5);
    for (int i = 0; i < 5; i++)
    {
      float f = i;
      positions.push_back({f, f, f});
      names.push_back("soa_node" + std::to_string(i));
    }

    ecs::create_entities(mgr,
      template2,
      {{
          {"position", std::move(positions)},
          {"name", std::move(names)}
      }}
    );
  }

  // assert(mgr.destroy_entity(eid1));

  ecs::perform_stage(mgr, "");
  ecs::perform_stage(mgr, "editor_act");
  query_test(mgr);

  std::vector<ecs::EntityId> allEids;

  for (int i = 0, n = mgr.entityContainer.entityRecords.size(); i < n; i++)
  {
    ecs::EntityId &eid = allEids.emplace_back();
    eid.entityIndex = i;
    eid.generation = mgr.entityContainer.entityRecords[i].generation;
  }


  query_by_eid_test(mgr, allEids);


  printf("ecs::send_event_immediate broadcast\n");
  ecs::send_event_immediate(mgr, UpdateEvent{});
  printf("ecs::send_event_immediate unicast\n");
  for (ecs::EntityId eid : allEids)
    ecs::send_event_immediate(mgr, eid, UpdateEvent{});

  printf("ecs::send_event broadcast\n");
  ecs::send_event(mgr, UpdateEvent{});
  printf("ecs::send_event unicast\n");
  for (ecs::EntityId eid : allEids)
    ecs::send_event(mgr, eid, UpdateEvent{});

  printf("ecs::perform_delayed_events\n");
  ecs::perform_delayed_events(mgr);

  std::vector<int> data(100);
  HeavyEvent heavyEvent{data};
  ecs::send_event_immediate(mgr, heavyEvent);
  ecs::send_event_immediate(mgr, HeavyEvent{data});

  ecs::send_event(mgr, HeavyEvent{heavyEvent});
  ecs::send_event(mgr, HeavyEvent{data});
  ecs::perform_delayed_events(mgr);

  for (ecs::EntityId eid : allEids)
  {
    ecs::set_component<int>(mgr, eid, "health", 25);
  }

  ecs::track_changes(mgr);

  ecs::destroy_entities(mgr);

  return 0;
}