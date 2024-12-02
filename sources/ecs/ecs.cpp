#include "ecs/ecs.h"
#include "ecs/config.h"
#include "ecs/query_iteration.h"
#include "ecs/ecs_manager.h"
#include "ecs/codegen_attributes.h"
#include <assert.h>


#define UNUSED(x) (void)(x)

#include "../../math_helper.h"


#include "../../timer.h"

void query_test(ecs::EcsManager &mgr);

ECS_TYPE_DECLARATION(int)
ECS_TYPE_DECLARATION(float3)
ECS_TYPE_DECLARATION_ALIAS(std::string, "string")

ECS_TYPE_REGISTRATION(ecs::EntityId)
ECS_TYPE_REGISTRATION(int)
ECS_TYPE_REGISTRATION(float3)
ECS_TYPE_REGISTRATION(std::string)

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

  ecs::TypeDeclarationInfo::iterate_all([&](const ecs::TypeDeclaration &type_declaration) {
    mgr.typeMap[type_declaration.typeId] = type_declaration;
  });


  for (const auto &[id, type] : mgr.typeMap)
  {
    printf("[ECS] type: %s, typeId: %x\n", type.typeName.c_str(), type.typeId);
  }

  ecs::ComponentId positionId = ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "position");
  ecs::ComponentId velocityId = ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<float3>::typeId, "velocity");
  ecs::ComponentId healthId = ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<int>::typeId, "health");
  ecs::ComponentId nameId = ecs::get_or_add_component(mgr.componentMap, ecs::TypeInfo<std::string>::typeId, "name");

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

  ecs::CodegenFileRegistration::register_all_codegen_files(mgr);


  //unfortunately, we can't use initializer_list here, because it doesn't support move semantics
  // _CONSTEXPR20 vector(initializer_list<_Ty> _Ilist, const _Alloc& _Al = _Alloc())

  ecs::InitializerList args;
  args.push_back(ecs::ComponentInit{"position", float3{1, 2, 3}});
  args.push_back(ecs::ComponentInit{"velocity", float3{4, 5, 6}});
  mgr.create_entity(template1, std::move(args));


  mgr.create_entity(
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
          {"position", float3{f, f, f}},
          // {velocityId, float3{f, f, f}},
          {"name", std::string("very_long_node_name") + std::to_string(i)}
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
          {"position", std::move(positions)},
          {"name", std::move(names)}
      }}
    );
  }

  // assert(mgr.destroy_entity(eid1));

  for (auto &[hashId, system] : mgr.systems)
  {
    ecs::perform_system(mgr.archetypeMap, system);
  }
  query_test(mgr);


  return 0;
}