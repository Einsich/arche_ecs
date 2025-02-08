#include <iostream>
#include <ecs/ecs.h>
#include "timer.h"
#include "math_helper.h"

#include <vector>
#include <algorithm>
#include <random>
#include <array>
#include <fstream>
#include <assert.h>

const float dt = 0.02f;
struct IBody
{
  // std::array<float, 128 * 2> data;
  virtual void update(float dt) = 0;
  virtual ~IBody() = default;
};


struct Body1 : public IBody
{
  float3 position;

  void update(float ) override
  {
    position = position + float3{1, 2, 3};
  }
};

static void update1(float3 &position)
{
  position = position + float3{1, 2, 3};
}


// static_assert(sizeof(Body1) == 24);

struct Body2 : public IBody
{
  float3 position;
  float3 velocity;

  void update(float dt) override
  {
    position = position + velocity * dt;
  }
};

// static_assert(sizeof(Body2) == 32);

static void update2(float3 &position, const float3 &velocity)
{
  position = position + velocity * dt;
}

struct Body3 : public IBody
{
  float3 position;
  float3 velocity;
  float3 acceleration;

  void update(float dt) override
  {
    velocity = acceleration * dt;
    position = position + velocity * dt;
  }
};

static void update3(float3 &position, float3 &velocity, const float3 &acceleration)
{
  velocity = acceleration * 0.02f;
  position = position + velocity * 0.02f;
}

void body1_array_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  Timer timer("Body1 Array");

  std::vector<Body1> bodies(N);

  for (int j = 0; j < TESTS_COUNT; j++)
  {
    for (int i = 0; i < N; i++)
    {
      Body1 &body = bodies[i];
      update1(body.position);
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body2_array_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  Timer timer("Body2 Array");

  std::vector<Body2> bodies(N);

  for (int j = 0; j < TESTS_COUNT; j++)
  {
    for (int i = 0; i < N; i++)
    {
      Body2 &body = bodies[i];
      update2(body.position, body.velocity);
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body3_array_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  Timer timer("Body3 Array");

  std::vector<Body3> bodies(N);

  for (int j = 0; j < TESTS_COUNT; j++)
  {
    for (int i = 0; i < N; i++)
    {
      Body3 &body = bodies[i];
      update3(body.position, body.velocity, body.acceleration);
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body1_pointers_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  std::vector<std::unique_ptr<Body1>> bodies1(N);
  for (int i = 0; i < N; i++)
  {
    bodies1[i].reset(new Body1());
  }

  auto rng = std::default_random_engine {};
  std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

  Timer timer("Body1 Pointers");
  for (int j = 0 ; j < TESTS_COUNT; j++)
  {
    for (int i = 0; i < N; i++)
    {
      update1(bodies1[i]->position);
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body2_pointers_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  std::vector<std::unique_ptr<Body2>> bodies1(N);
  for (int i = 0; i < N; i++)
  {
    bodies1[i].reset(new Body2());
  }
  auto rng = std::default_random_engine {};
  std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

  Timer timer("Body2 Pointers");
  for (int j = 0 ; j < TESTS_COUNT; j++)
  {
    for (int i = 0; i < N; i++)
    {
      Body2 *body = bodies1[i].get();
      update2(body->position, body->velocity);
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body3_pointers_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  std::vector<std::unique_ptr<Body3>> bodies1(N);
  for (int i = 0; i < N; i++)
  {
    bodies1[i].reset(new Body3());
  }

  auto rng = std::default_random_engine {};
  std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

  Timer timer("Body3 Pointers");
  for (int j = 0 ; j < TESTS_COUNT; j++)
  {
    for (int i = 0; i < N; i++)
    {
      Body3 *body = bodies1[i].get();
      update3(body->position, body->velocity, body->acceleration);
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body1_virtual_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  std::vector<std::unique_ptr<IBody>> bodies1(N);
  for (int i = 0; i < N; i++)
  {
    bodies1[i].reset(new Body1());
  }

  auto rng = std::default_random_engine {};
  std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

  Timer timer("IBody1 Virtual");
  for (int j = 0 ; j < TESTS_COUNT; j++)
  {
    for (int i = 0; i < N; i++)
    {
      bodies1[i]->update(dt);
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body2_virtual_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  std::vector<std::unique_ptr<IBody>> bodies1(N);
  for (int i = 0; i < N; i++)
  {
    bodies1[i].reset(new Body2());
  }

  auto rng = std::default_random_engine {};
  std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

  Timer timer("IBody2 Virtual");
  for (int j = 0 ; j < TESTS_COUNT; j++)
  {
    for (int i = 0; i < N; i++)
    {
      bodies1[i]->update(dt);
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body3_virtual_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  std::vector<std::unique_ptr<IBody>> bodies1(N);
  for (int i = 0; i < N; i++)
  {
    bodies1[i].reset(new Body3());
  }

  auto rng = std::default_random_engine {};
  std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

  Timer timer("IBody3 Virtual");
  for (int j = 0 ; j < TESTS_COUNT; j++)
  {
    for (int i = 0; i < N; i++)
    {
      bodies1[i]->update(dt);
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body3_soa_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  std::vector<float3> positionsStorage(N);
  std::vector<float3> velocitiesStorage(N);
  std::vector<float3> accelerationsStorage(N);

  Timer timer("Body3 SoA Split Data");
  for (int j = 0 ; j < TESTS_COUNT; j++)
  {
    float3 * __restrict__ positions = positionsStorage.data();
    float3 * __restrict__ velocities  = velocitiesStorage.data();
    const float3 * __restrict__ accelerations = accelerationsStorage.data();

    for (int i = 0; i < N; i++)
    {
      update3(*positions, *velocities, *accelerations);

      positions += 1;
      velocities += 1;
      accelerations += 1;
    }
  }
  benchmark_file << timer.stop() << ";";
}

void body3_merged_soa_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  std::vector<float3> float3Storage(N * 3);

  Timer timer("Body3 SoA One Data");
  for (int j = 0 ; j < TESTS_COUNT; j++)
  {
    float3 * __restrict__ positions = float3Storage.data() + 0;
    float3 * __restrict__ velocities = float3Storage.data() + N;
    const float3 * __restrict__ accelerations = float3Storage.data() + N * 2;

    for (int i = 0; i < N; i++)
    {
      update3(*positions, *velocities, *accelerations);
      positions += 1;
      velocities += 1;
      accelerations += 1;
    }
  }
  benchmark_file << timer.stop() << ";";
}

void ecs_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N);

void run_all_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  body1_array_test(benchmark_file, TESTS_COUNT, N);
  body2_array_test(benchmark_file, TESTS_COUNT, N);
  body3_array_test(benchmark_file, TESTS_COUNT, N);

  body1_pointers_test(benchmark_file, TESTS_COUNT, N);
  body2_pointers_test(benchmark_file, TESTS_COUNT, N);
  body3_pointers_test(benchmark_file, TESTS_COUNT, N);

  body1_virtual_test(benchmark_file, TESTS_COUNT, N);
  body2_virtual_test(benchmark_file, TESTS_COUNT, N);
  body3_virtual_test(benchmark_file, TESTS_COUNT, N);

  body3_soa_test(benchmark_file, TESTS_COUNT, N);
  body3_merged_soa_test(benchmark_file, TESTS_COUNT, N);

  ecs_test(benchmark_file, TESTS_COUNT, N);
}

int main(int argc, char *argv[])
{
  int TESTS_COUNT = 1;
  std::string output_file = "benchmark.csv";
  bool parsed = argc > 1;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg.starts_with("--repeat_count")) {
      int value = std::atoi(argv[i] + sizeof("--repeat_count"));
      assert(value > 0);
      TESTS_COUNT = value;
    } else if (arg.starts_with("--output")) {
      output_file = argv[i] + sizeof("--output");
    } else {
      parsed = false;
    }
  }

  if (!parsed) {
    std::cout << "Usage: " << argv[0] << " [--repeat_count=<int>] [--output=<string>]" << std::endl;
    return 1;
  }



  std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  std::ofstream benchmark_file;
  benchmark_file.open(output_file);
  benchmark_file <<
    "count;" <<
    "body1_array;" << "body2_array;" << "body3_array;" <<
    "body1_pointers;" << "body2_pointers;" << "body3_pointers;" <<
    "body1_virtual;" << "body2_virtual;" << "body3_virtual;" <<
    "body3_soa;" << "body3_merged_soa;" <<
    "body1_query;" << "body2_query;" << "body3_query;" <<
    "body1_system;" << "body2_system;" << "body3_system;" <<
    std::ctime(&currentTime);

  {
    //Warmup
    std::ofstream stub_benchmark_file;
    run_all_test(stub_benchmark_file, 1, 100);
  }
  // explore perf for entities less than 1K
  for (int N = 1; N < 1'000; N += 10) // 1K iterations
  {
    benchmark_file << N << ';';
    run_all_test(benchmark_file, TESTS_COUNT, N);
    benchmark_file << '\n';
  }
  // and for more serious entities amount
  for (int N = 1'000; N < 100'000; N += 1000) // 1K iterations
  {
    benchmark_file << N << ';';
    run_all_test(benchmark_file, TESTS_COUNT, N);
    benchmark_file << '\n';
  }
}


ECS_TYPE_DECLARATION(float3)
ECS_TYPE_DECLARATION(int)
ECS_TYPE_DECLARATION_ALIAS(std::string, "string")



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


ECS_TYPE_REGISTRATION(ecs::EntityId)
ECS_TYPE_REGISTRATION(int)
ECS_TYPE_REGISTRATION(float3)
ECS_TYPE_REGISTRATION(std::string)

static ecs::System *find_system_by_name(ecs::EcsManager &mgr, const char *name)
{
  for (auto &[id, systems] : mgr.systems)
  {
    for (auto &system : systems)
    {
      if (system.name == name)
      {
        return &system;
      }
    }
  }
  return nullptr;
}

void ecs_test(std::ofstream &benchmark_file, int TESTS_COUNT, int N)
{
  ecs::EcsManager mgr;

  ecs::register_all_type_declarations(mgr);

  ecs::register_all_codegen_files(mgr);

  ecs::sort_systems(mgr);

  ecs::TemplateId bodyTemplate = template_registration(mgr, "body",
    {mgr, {
      {mgr, "position", float3{}},
      {mgr, "velocity", float3{}},
      {mgr, "acceleration", float3{}},
    }}, ecs::ArchetypeChunkSize::Thousands);

  {
    const int entityCount = N / 2;
    Timer timer("create_entity"); // (0.066700 ms)
    for (int i = 0; i < entityCount; i++)
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
  }
  {
    const int entityCount = N / 2;
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

    ecs::create_entities(mgr,
      bodyTemplate,
      std::move(init)
    );
  }

  {
    Timer timer("query_perf_test1"); // (0.046000 ms)
    for (int j = 0 ; j < TESTS_COUNT; j++)
    {
      query_perf_test1(mgr);
    }
    benchmark_file << timer.stop() << ";";
  }
  {
    Timer timer("query_perf_test2"); // (0.046000 ms)
    for (int j = 0 ; j < TESTS_COUNT; j++)
    {
      query_perf_test2(mgr);
    }
    benchmark_file << timer.stop() << ";";
  }

  {
    Timer timer("query_perf_test3"); // (0.046000 ms)
    for (int j = 0 ; j < TESTS_COUNT; j++)
    {
      query_perf_test3(mgr);
    }
    benchmark_file << timer.stop() << ";";
  }

  {
    ecs::System *system_perf_test1 = find_system_by_name(mgr, "system_perf_test1");
    Timer timer("system_perf_test1");
    for (int j = 0 ; j < TESTS_COUNT; j++)
    {
      ecs::perform_system(*system_perf_test1);
    }
    benchmark_file << timer.stop() << ";";
  }
  {
    ecs::System *system_perf_test2 = find_system_by_name(mgr, "system_perf_test2");
    Timer timer("system_perf_test2");
    for (int j = 0 ; j < TESTS_COUNT; j++)
    {
      ecs::perform_system(*system_perf_test2);
    }
    benchmark_file << timer.stop() << ";";
  }
  {
    ecs::System *system_perf_test3 = find_system_by_name(mgr, "system_perf_test3");
    Timer timer("system_perf_test3");
    for (int j = 0 ; j < TESTS_COUNT; j++)
    {
      ecs::perform_system(*system_perf_test3);
    }
    benchmark_file << timer.stop() << ";";
  }
  ecs::destroy_entities(mgr);
}