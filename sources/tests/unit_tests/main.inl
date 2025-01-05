#include "ecs/ecs.h"
#include "ecs/ecs_manager.h"
#include <assert.h>
#include "math_helper.h"
#include "timer.h"

void query_test(ecs::EcsManager &mgr);

ECS_TYPE_DECLARATION(int)
ECS_TYPE_DECLARATION(float3)
ECS_TYPE_DECLARATION_ALIAS(std::string, "string")


ECS_TYPE_REGISTRATION(int)
ECS_TYPE_REGISTRATION(float3)
ECS_TYPE_REGISTRATION(std::string)


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

ECS_EVENT() on_appear_event(const ecs::OnAppear &, const std::string &name, const int *health)
{
  printf("on_appear_event [%s] %d\n", name.c_str(), health ? *health : -1);
}

ECS_EVENT() on_disappear_event(const ecs::OnDisappear &, const std::string &name, const int *health)
{
  printf("on_disappear_event [%s] %d\n", name.c_str(), health ? *health : -1);
}

ECS_EVENT(on_event=ecs::OnAppear, ecs::OnDisappear)
appear_disapper_event(const ecs::Event &event, const std::string &name, const int *health)
{
  if (const ecs::OnAppear *updateEvent = event.cast<ecs::OnAppear>())
    printf("appear_disapper_event OnAppear [%s] %d\n", name.c_str(), health ? *health : -1);
  else if (const ecs::OnDisappear *heavyEvent = event.cast<ecs::OnDisappear>())
    printf("appear_disapper_event OnDisappear [%s] %d\n", name.c_str(), health ? *health : -1);
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

int main()
{
  const bool EntityContainerTest = true;
  if (EntityContainerTest)
  {
    ecs_details::EntityContainer entityContainer;
    ecs::EntityId entityId = entityContainer.create_entity(0, 0);
    assert(entityContainer.is_alive(entityId));
    entityContainer.destroy_entity(entityId);
    assert(!entityContainer.is_alive(entityId));
    assert(!entityContainer.is_alive(ecs::EntityId()));

    ecs::EntityId entityId1 = entityContainer.create_entity(0, 0);
    assert(entityContainer.is_alive(entityId1));
    assert(entityId != entityId1);
    ECS_UNUSED(entityId);
    ECS_UNUSED(entityId1);
  }
  ecs::EcsManager mgr;

  ecs::register_all_type_declarations(mgr);

  ecs::register_all_codegen_files(mgr);

  for (const auto &[id, type] : mgr.typeMap)
  {
    printf("[ECS] type: %s, typeId: %x\n", type.typeName.c_str(), type.typeId);
  }

  ecs::ComponentId positionId = ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "position");
  ecs::ComponentId velocityId = ecs::get_or_add_component(mgr, ecs::TypeInfo<float3>::typeId, "velocity");
  ecs::ComponentId healthId = ecs::get_or_add_component(mgr, ecs::TypeInfo<int>::typeId, "health");
  ecs::ComponentId nameId = ecs::get_or_add_component(mgr, ecs::TypeInfo<std::string>::typeId, "name");

  for (const auto &[id, component] : mgr.componentMap)
  {
    printf("[ECS] component: %s, componentId: %x, typeId: %x\n", component->name.c_str(), component->componentId, component->typeId);
  }

  ecs::TemplateId template1 = template_registration(mgr, "point",
    {{
      {positionId, float3{}},
      {velocityId, float3{}}
    }}, ecs::ArchetypeChunkSize::Dozens);

  ecs::TemplateId template2 = template_registration(mgr, "named_point",
    {{
      {velocityId, float3{}},
      {positionId, float3{}},
      {nameId, std::string{}}
    }}, ecs::ArchetypeChunkSize::Dozens);

  ecs::TemplateId template3 = template_registration(mgr, "named_alive_point",
    {{
      {velocityId, float3{}},
      {positionId, float3{}},
      {nameId, std::string{}},
      {healthId, 0}
    }}, ecs::ArchetypeChunkSize::Dozens);

  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    printf("[ECS] archetype: %x, components: %zu\n", archetype->archetypeId, archetype->collumns.size());
  }

  ecs::InitializerList args;
  args.push_back(ecs::ComponentInit{"position", float3{1, 2, 3}});
  args.push_back(ecs::ComponentInit{"velocity", float3{4, 5, 6}});

  {
    ecs::EntityId eid = ecs::create_entity(mgr, template1, std::move(args));
    ECS_UNUSED(eid);
    const float3 positionValue = float3{1, 2, 3};
    const float3 newPositionValue = float3{3, 2, 1};
    float3 newPositionValueRW = float3{3, 2, 1};
    assert(*ecs::get_component<float3>(mgr, eid, "position") == positionValue);
    assert(*ecs::get_rw_component<float3>(mgr, eid, "position") == positionValue);
    assert(ecs::get_component<int>(mgr, eid, "position") == nullptr);
    assert(ecs::get_component<float3>(mgr, eid, "positions") == nullptr);
    assert(ecs::set_component<float3>(mgr, eid, "position", newPositionValue) == true);
    assert(ecs::set_component<float3>(mgr, eid, "position", newPositionValueRW) == true);
    assert(ecs::set_component<float3>(mgr, eid, "position", float3{newPositionValue}) == true);
    assert(*ecs::get_component<float3>(mgr, eid, "position") == newPositionValue);
  }


  ecs::create_entity(mgr,
    template3,
    ecs::InitializerList{
      {
        ecs::ComponentInit{"position", float3{7, 8, 9}},
        ecs::ComponentInit{"velocity", float3{10, 11, 12}},
        ecs::ComponentInit{"health", 100},
        ecs::ComponentInit{"name", std::string("entity1")}
      }
    }
  );

  ecs::TemplateId movableTemplate = template_registration(mgr, "movable",
    {{
      {positionId, float3{0, 0, 0}},
      {velocityId, float3{0, 0, 0}}
    }}
  );
  ecs::TemplateId brickTemplate = template_registration(mgr, "brick", movableTemplate,
    {{
      {positionId, float3{1, 0, 0}},
      {healthId, 10},
      {nameId, std::string("brick")}
    }}
  );
  ecs::TemplateId humanTemplate = template_registration(mgr, "human", movableTemplate,
    {{
      {positionId, float3{2, 0, 0}},
      // {healthId, 50},
      {nameId, std::string("human")}
    }}
  );
  ecs::TemplateId bigBrickTemplate = template_registration(mgr, "bigbrick", brickTemplate,
    {{
      {positionId, float3{3, 0, 0}},
      {healthId, 100}
    }}
  );

  ecs::create_entity(mgr, movableTemplate);
  ecs::create_entity(mgr, brickTemplate);
  ecs::create_entity(mgr, humanTemplate);
  ecs::create_entity(mgr, bigBrickTemplate);


  // if (false)
  {
    Timer timer("create_entity"); // (0.066700 ms)
    for (int i = 0; i < 5; i++)
    {
      float f = i;
      ecs::EntityId eid = ecs::create_entity(mgr,
        template2,
        {{
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
    ecs::InitializerSoaList init;
    ecs::create_entities(mgr,
      template2,
      {{
          {"position", std::move(positions)},
          {"name", std::move(names)}
      }}
    );
  }

  // assert(mgr.destroy_entity(eid1));

  for (auto &[hashId, system] : mgr.systems)
  {
    ecs::perform_system(system);
  }
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

  ecs::destroy_entities(mgr);

  return 0;
}