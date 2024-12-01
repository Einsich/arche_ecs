#include "ecs/ecs.h"
#include "ecs/config.h"
#include "ecs/query_iteration.h"
#include "ecs/ecs_manager.h"
#include <assert.h>


#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#define UNUSED(x) (void)(x)

#include "../../math_helper.h"

// user_code.cpp.inl

void update(ecs::EntityId eid, float3 &position, const float3 &velocity)
{
  printf("update [%d/%d] (%f %f %f), (%f %f %f)\n", eid.entityIndex, eid.generation, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
}


void print_name(const std::string &name, float3 position, int *health)
{
  printf("print_name [%s] (%f %f %f), %d\n", name.c_str(), position.x, position.y, position.z, health ? *health : -1);
}

// user_code.cpp.gen
// #include "user_code.cpp.inl"

// user_code.cpp.gen
// #include "user_code.cpp.inl"


void update_archetype_codegen(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  ecs::templated_archetype_iterate<N, ecs::EntityId *, float3 *, const float3 *>(archetype, to_archetype_component, update, std::make_index_sequence<N>());
}

void print_name_archetype_codegen(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  ecs::templated_archetype_iterate<N, std::string *, float3 *, ecs::PrtWrapper<int>>(archetype, to_archetype_component, print_name, std::make_index_sequence<N>());
}


#include "../../timer.h"

template<typename Callable>
static void print_name(ecs::EcsManager &, Callable &&);

int main2()
{
  const bool EntityContainerTest = false;
  if (EntityContainerTest)
  {
    ecs::EntityContainer entityContainer;
    ecs::EntityId entityId = entityContainer.create_entity(0, 0);
    assert(entityContainer.is_alive(entityId));
    entityContainer.destroy_entity(entityId);
    assert(!entityContainer.is_alive(entityId));
    assert(!entityContainer.is_alive(ecs::EntityId()));

    ecs::EntityId entityId1 = entityContainer.create_entity(0, 0);
    assert(entityContainer.is_alive(entityId1));
    assert(entityId != entityId1);
    UNUSED(entityId);
    UNUSED(entityId1);
  }
  ecs::EcsManager mgr;

  ecs::TypeId float3Id = ecs::type_registration<float3>(mgr.typeMap, "float3");
  ecs::TypeId intId = ecs::type_registration<int>(mgr.typeMap, "int");
  ecs::TypeId stringId = ecs::type_registration<std::string>(mgr.typeMap, "string");

  for (const auto &[id, type] : mgr.typeMap)
  {
    printf("[ECS] type: %s, typeId: %x\n", type->typeName.c_str(), type->typeId);
  }

  ecs::ComponentId positionId = ecs::component_registration(mgr.componentMap, ecs::hash("float3")/* float3Id */, "position");
  ecs::ComponentId velocityId = ecs::component_registration(mgr.componentMap, float3Id, "velocity");
  ecs::ComponentId healthId = ecs::component_registration(mgr.componentMap, intId, "health");
  ecs::ComponentId nameId = ecs::component_registration(mgr.componentMap, stringId, "name");

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
    printf("[ECS] archetype: %x, components: %zu\n", archetype.archetypeId, archetype.collumns.size());
  }


  ecs::Query update_position_query;
  ecs::Query print_name_query;

  {
    ecs::Query query;
    query.uniqueName = __FILE__ LINE_STRING "update_position";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature = {
      {mgr.eidComponentId, ecs::Query::ComponentAccess::READ_COPY},
      {positionId, ecs::Query::ComponentAccess::READ_WRITE},
      {velocityId, ecs::Query::ComponentAccess::READ_ONLY}
    };
    query.requireComponents = {};
    query.excludeComponents = {healthId};
    query.update_archetype = update_archetype_codegen;

    for (auto &[id, archetype]  : mgr.archetypeMap)
    {
      query.try_registrate(archetype);
    }
    // mgr.queries[query.nameHash] = std::move(query); // TODO: fix this
    mgr.queries[query.nameHash] = query;
    update_position_query = std::move(query);
  }
  {
    ecs::Query query;
    query.uniqueName = __FILE__ LINE_STRING "print_name";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature = {
      {nameId, ecs::Query::ComponentAccess::READ_ONLY},
      {positionId, ecs::Query::ComponentAccess::READ_COPY},
      {healthId, ecs::Query::ComponentAccess::READ_ONLY_OPTIONAL},
    };
    query.requireComponents = {};
    query.excludeComponents = {};
    query.update_archetype = print_name_archetype_codegen;
    for (auto &[id, archetype]  : mgr.archetypeMap)
    {
      query.try_registrate(archetype);
    }
    // mgr.queries[query.nameHash] = std::move(query); // TODO: fix this
    mgr.queries[query.nameHash] = query;
    print_name_query = std::move(query);
  }
  {
    ecs::Query query;
    query.uniqueName = "print_name_query";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature = {
      {nameId, ecs::Query::ComponentAccess::READ_ONLY},
      {healthId, ecs::Query::ComponentAccess::READ_ONLY_OPTIONAL}
    };
    query.requireComponents = {};
    query.excludeComponents = {};
    query.update_archetype = nullptr;

    for (auto &[id, archetype]  : mgr.archetypeMap)
    {
      query.try_registrate(archetype);
    }
    mgr.queries[query.nameHash] = std::move(query);
  }

  //unfortunately, we can't use initializer_list here, because it doesn't support move semantics
  // _CONSTEXPR20 vector(initializer_list<_Ty> _Ilist, const _Alloc& _Al = _Alloc())

  ecs::InitializerList args;
  args.push_back(ecs::ComponentInit{positionId, float3{1, 2, 3}});
  args.push_back(ecs::ComponentInit{velocityId, float3{4, 5, 6}});
  mgr.create_entity(template1, std::move(args));


  mgr.create_entity(
    template3,
    ecs::InitializerList{
      {
        ecs::ComponentInit{positionId, float3{7, 8, 9}},
        ecs::ComponentInit{velocityId, float3{10, 11, 12}},
        ecs::ComponentInit{healthId, 100},
        ecs::ComponentInit{nameId, std::string("entity1")}
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

  mgr.create_entity(movableTemplate);
  mgr.create_entity(brickTemplate);
  mgr.create_entity(humanTemplate);
  mgr.create_entity(bigBrickTemplate);


  // if (false)
  {
    Timer timer("create_entity"); // (0.066700 ms)
    for (int i = 0; i < 100; i++)
    {
      float f = i;
      ecs::EntityId eid = mgr.create_entity(
        template2,
        {{
          {positionId, float3{f, f, f}},
          // {velocityId, float3{f, f, f}},
          {nameId, std::string("very_long_node_name") + std::to_string(i)}
        }}
      );
      UNUSED(eid);
    }
  }

  // if (false)
  {
    Timer timer("create_entities"); // (0.046000 ms)
    std::vector<float3> positions;
    std::vector<std::string> names;
    positions.reserve(100);
    names.reserve(100);
    for (int i = 0; i < 100; i++)
    {
      float f = i;
      positions.push_back({f, f, f});
      names.push_back("soa_node" + std::to_string(i));
    }
    ecs::InitializerSoaList init;
    mgr.create_entities(
      template2,
      {{
          {positionId, std::move(positions)},
          {nameId, std::move(names)}
      }}
    );
  }

  // assert(mgr.destroy_entity(eid1));

  ecs::update_query(mgr.archetypeMap, update_position_query);
  ecs::update_query(mgr.archetypeMap, print_name_query);

  print_name(mgr, [](const std::string &name, int *health)
  {
    printf("print_name [%s] %d\n", name.c_str(), health ? *health : -1);
  });


  return 0;
}

template<typename Callable>
static void print_name(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("print_name_query");
  ecs::call_query(mgr, queryHash, [&](ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
  {
    const int N = 2;
    ecs::templated_archetype_iterate<N, std::string *, ecs::PrtWrapper<int>>(archetype, to_archetype_component, std::move(query_function), std::make_index_sequence<N>());
  });
}