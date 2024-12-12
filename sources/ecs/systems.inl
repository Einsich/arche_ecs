#include "ecs/ecs_manager.h"
#include "ecs/codegen_attributes.h"

#include "../../math_helper.h"

ECS_TYPE_DECLARATION(float3)
ECS_TYPE_DECLARATION(int)
ECS_TYPE_DECLARATION_ALIAS(std::string, "string")
static_assert(sizeof(float3) == 0xc);
ECS_SYSTEM() update(ecs::EntityId eid, float3 &position, const float3 &velocity)
{
  printf("update [%d/%d] (%f %f %f), (%f %f %f)\n", eid.entityIndex, eid.generation, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
}


ECS_SYSTEM() print_name(const std::string &name, float3 position, int *health)
{
  printf("print_name [%s] (%f %f %f), %d\n", name.c_str(), position.x, position.y, position.z, health ? *health : -1);
}

template<typename Callable>
static void print_name_query(ecs::EcsManager &, Callable &&);

void query_test(ecs::EcsManager &mgr)
{
  ECS_QUERY() print_name_query(mgr, [](const std::string &name, int *health)
  {
    printf("print_name [%s] %d\n", name.c_str(), health ? *health : -1);
  });
}

const float dt = 0.02f;

ECS_SYSTEM() system_perf_test1(float3 &position)
{
  position = position + float3{1, 2, 3};
}


ECS_SYSTEM() system_perf_test2(float3 &position, const float3 &velocity)
{
  position = position + velocity * dt;
}

ECS_SYSTEM() system_perf_test3(float3 &position, float3 &velocity, const float3 &acceleration)
{
  velocity = acceleration * dt;
  position = position + velocity * dt;
}

template<typename Callable>
static void query_perf_test1(ecs::EcsManager &, Callable &&);
template<typename Callable>
static void query_perf_test2(ecs::EcsManager &, Callable &&);
template<typename Callable>
static void query_perf_test3(ecs::EcsManager &, Callable &&);

void query_perf_test1(ecs::EcsManager &mgr)
{
  ECS_QUERY() query_perf_test1(mgr, [](float3 &position)
  {
    position = position + float3{1, 2, 3};
  });
}

void query_perf_test2(ecs::EcsManager &mgr)
{
  ECS_QUERY() query_perf_test2(mgr, [](float3 &position, const float3 &velocity)
  {
    position = position + velocity * dt;
  });
}

void query_perf_test3(ecs::EcsManager &mgr)
{
  ECS_QUERY() query_perf_test3(mgr, [](float3 &position, float3 &velocity, const float3 &acceleration)
  {
    velocity = acceleration * dt;
    position = position + velocity * dt;
  });
}

#include "../../timer.h"

ECS_TYPE_REGISTRATION(ecs::EntityId)
ECS_TYPE_REGISTRATION(int)
ECS_TYPE_REGISTRATION(float3)
ECS_TYPE_REGISTRATION(std::string)


void main3(int TEST_COUNT, int entityCount)
{
  entityCount /= 2;
  ecs::EcsManager mgr;

  ecs::TypeDeclarationInfo::iterate_all([&](const ecs::TypeDeclaration &type_declaration) {
    mgr.typeMap[type_declaration.typeId] = type_declaration;
  });

  ecs::CodegenFileRegistration::register_all_codegen_files(mgr);
  ecs::TemplateId bodyTemplate = template_registration(mgr, "body",
    {{
      {mgr, "position", float3{}},
      {mgr, "velocity", float3{}},
      {mgr, "acceleration", float3{}},
    }}, ecs::ArchetypeChunkSize::Thousands);

  {
    Timer timer("create_entity"); // (0.066700 ms)
    for (int i = 0; i < entityCount; i++)
    {
      float f = i;
      mgr.create_entity(
        bodyTemplate,
        {{
          {"position", float3{f, f, f}},
          {"velocity", float3{f, f, f}},
          {"acceleration", float3{1.f, 1.f, 1.f}}
        }}
      );
    }
  }
  {
    Timer timer("create_entities"); // (0.046000 ms)
    std::vector<float3> positions;
    std::vector<float3> velocities;
    std::vector<float3> accelerations;
    positions.reserve(entityCount);
    velocities.reserve(entityCount);
    accelerations.reserve(entityCount);
    for (int i = 0; i < entityCount; i++)
    {
      float f = i;
      positions.push_back({f, f, f});
      velocities.push_back({f, f, f});
      accelerations.push_back({1.f, 1.f, 1.f});
    }

    // this code id too unoptimal, because of ComponentDataSoa implementation
    ecs::InitializerSoaList init =
      {{
          {mgr, "position", std::move(positions)},
          {mgr, "velocity", std::move(velocities)},
          {mgr, "acceleration", std::move(accelerations)},
      }};

    mgr.create_entities(
      bodyTemplate,
      std::move(init)
    );
  }

  {
    Timer timer("query_perf_test1"); // (0.046000 ms)
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      query_perf_test1(mgr);
    }
  }
  {
    Timer timer("query_perf_test2"); // (0.046000 ms)
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      query_perf_test2(mgr);
    }
  }

  {
    Timer timer("query_perf_test3"); // (0.046000 ms)
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      query_perf_test3(mgr);
    }
  }

  {
    Timer timer("system_perf_test1");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      auto it = mgr.systems.find(ecs::hash("sources/ecs/systems.inl:34[system_perf_test1]"));
      if (it != mgr.systems.end())
      {
        ecs::System &system = it->second;
        ecs::perform_system(system);
      }
    }
  }
  {
    Timer timer("system_perf_test2");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      auto it = mgr.systems.find(ecs::hash("sources/ecs/systems.inl:40[system_perf_test2]"));
      if (it != mgr.systems.end())
      {
        ecs::System &system = it->second;
        ecs::perform_system(system);
      }
    }
  }
  {
    Timer timer("system_perf_test3");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      auto it = mgr.systems.find(ecs::hash("sources/ecs/systems.inl:45[system_perf_test3]"));
      if (it != mgr.systems.end())
      {
        ecs::System &system = it->second;
        ecs::perform_system(system);
      }
    }
  }
}
