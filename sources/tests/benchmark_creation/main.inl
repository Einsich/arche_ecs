#include <iostream>
#include <ecs/ecs.h>
#include "timer.h"
#include "math_helper.h"

#include <vector>
#include <algorithm>
#include <random>
#include <array>
#include <fstream>

void entity_creation_test(std::ofstream &benchmark_file, int N);

int main(int argc, char *argv[])
{
  std::string output_file = "benchmark_creation.csv";
  bool parsed = argc > 1;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg.starts_with("--output")) {
      output_file = argv[i] + sizeof("--output");
    } else {
      parsed = false;
    }
  }

  if (!parsed) {
    std::cout << "Usage: " << argv[0] << " [--output=<string>]" << std::endl;
    // return 1;
  }



  std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  std::ofstream benchmark_file;
  benchmark_file.open(output_file);
  benchmark_file <<
    "count;" <<
    "soa_creation;" << "aos_creation;" <<
    "create_entity_sync;" << "_outdated_;" <<
    "prepare_create_entities;" << "create_entities_sync;" <<
    "create_entity_async;" << "perform_delayed_entities_creation;" << "create_entity_async_total;" << "create_entities_async;" <<
    std::ctime(&currentTime);

  {
    //Warmup
    std::ofstream stub_benchmark_file;
    entity_creation_test(stub_benchmark_file, 100);
  }
  // explore perf for entities less than 1K
  for (int N = 1; N < 1'000; N += 10) // 1K iterations
  {
    benchmark_file << N << ';';
    entity_creation_test(benchmark_file, N);
    benchmark_file << '\n';
  }
  // and for more serious entities amount
  for (int N = 1'000; N < 100'000; N += 1000) // 1K iterations
  {
    benchmark_file << N << ';';
    entity_creation_test(benchmark_file, N);
    benchmark_file << '\n';
  }
}

ECS_TYPE_DECLARATION(float3)
ECS_TYPE_DECLARATION(int)
ECS_TYPE_DECLARATION_ALIAS(std::string, "string")

ECS_TYPE_REGISTRATION(ecs::EntityId)
ECS_TYPE_REGISTRATION(int)
ECS_TYPE_REGISTRATION(float3)
ECS_TYPE_REGISTRATION(std::string)

void entity_creation_test(std::ofstream &benchmark_file, int N)
{
  // sizeof(std::string);
  // sizeof(std::vector<int>);
  // sizeof(float[4]);
  // sizeof(float[16]);
  {
    Timer timer("soa_creation");
    std::vector<float3> positions;
    std::vector<float3> velocities;
    std::vector<float3> accelerations;
    positions.reserve(N);
    velocities.reserve(N);
    accelerations.reserve(N);
    for (int i = 0; i < N; i++)
    {
      float f = i;
      positions.push_back({f, f, f});
      velocities.push_back({f, f, f});
      accelerations.push_back({1.f, 1.f, 1.f});
    }
    benchmark_file << timer.stop() << ";";
  }

  {
    Timer timer("aos_creation");
    struct Body
    {
      float3 position;
      float3 velocity;
      float3 acceleration;
    };
    std::vector<Body> bodies;
    bodies.reserve(N);
    for (int i = 0; i < N; i++)
    {
      float f = i;
      bodies.push_back(Body{
        {f, f, f},
        {f, f, f},
        {1.f, 1.f, 1.f}
      });
    }
    benchmark_file << timer.stop() << ";";
  }
  ecs::EcsManager mgr;

  ecs::register_all_type_declarations(mgr);

  ecs::register_all_codegen_files(mgr);

  ecs::sort_systems(mgr);

  ecs::preallocate_initializers(mgr, N + 100, 3);

  ecs::TemplateId bodyTemplate = template_registration(mgr, "body",
    {mgr, {
      {mgr, "position", float3{}},
      {mgr, "velocity", float3{}},
      {mgr, "acceleration", float3{}},
    }}, ecs::ArchetypeChunkSize::Thousands);

  {
    Timer timer("create_entity_sync");
    for (int i = 0; i < N; i++)
    {
      float f = i;
      ecs::create_entity_sync(mgr,
        bodyTemplate,
        {mgr, {
          {"position", float3{f, f, f}},
          {"velocity", float3{f, f, f}},
          {"acceleration", float3{1.f, 1.f, 1.f}}
        }}
      );
    }
    benchmark_file << timer.stop() << ";";
  }
  {
    Timer timer("_outdated_");
    benchmark_file << timer.stop() << ";";
  }
  {
    Timer timer("create_entities");
    std::vector<float3> positions;
    std::vector<float3> velocities;
    std::vector<float3> accelerations;
    positions.reserve(N);
    velocities.reserve(N);
    accelerations.reserve(N);
    for (int i = 0; i < N; i++)
    {
      float f = i;
      positions.push_back({f, f, f});
      velocities.push_back({f, f, f});
      accelerations.push_back({1.f, 1.f, 1.f});
    }

    ecs::InitializerSoaList init =
      {{
          {mgr, "position", std::move(positions)},
          {mgr, "velocity", std::move(velocities)},
          {mgr, "acceleration", std::move(accelerations)},
      }};

    benchmark_file << timer.get_time() << ";";

    ecs::create_entities_sync(mgr,
      bodyTemplate,
      std::move(init)
    );
    benchmark_file << timer.stop() << ";";
  }

  {
    mgr.delayedEntities.reserve(N);
    Timer totalTimer("create_entity_async_total");
    {
      Timer timer("create_entity_async");
      for (int i = 0; i < N; i++)
      {
        float f = i;
        ecs::create_entity(mgr,
          bodyTemplate,
          {mgr, {
            {"position", float3{f, f, f}},
            {"velocity", float3{f, f, f}},
            {"acceleration", float3{1.f, 1.f, 1.f}}
          }}
        );
      }
      benchmark_file << timer.stop() << ";";
    }
    {
      Timer timer("perform_delayed_entities_creation");
      ecs::perform_delayed_entities_creation(mgr);
      benchmark_file << timer.stop() << ";";
    }
    benchmark_file << totalTimer.stop() << ";";
  }
  {
    Timer timer("create_entities_async");
    std::vector<float3> positions;
    std::vector<float3> velocities;
    std::vector<float3> accelerations;
    positions.reserve(N);
    velocities.reserve(N);
    accelerations.reserve(N);
    for (int i = 0; i < N; i++)
    {
      float f = i;
      positions.push_back({f, f, f});
      velocities.push_back({f, f, f});
      accelerations.push_back({1.f, 1.f, 1.f});
    }
    ecs::InitializerSoaList init =
      {{
          {mgr, "position", std::move(positions)},
          {mgr, "velocity", std::move(velocities)},
          {mgr, "acceleration", std::move(accelerations)},
      }};
    ecs::create_entities(mgr,
      bodyTemplate,
      std::move(init)
    );
    ecs::perform_delayed_entities_creation(mgr);
    benchmark_file << timer.stop() << ";";
  }

  ecs::destroy_entities(mgr);
}