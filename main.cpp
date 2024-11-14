#include <iostream>
#include "ecs/ecs.h"
#include "timer.h"
#include "math_helper.h"

#include <vector>
#include <algorithm>
#include <random>

struct IBody
{
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

// static_assert(sizeof(Body3) == 48);

int main2();

int main()
{
  main2();
  return 0;
  Timer timer("Main");
  // position
  // position, velocity
  // position, velocity, acceleration
  const float dt = 0.02f;
  const int N1 = 100000;
  // const int N2 = 1000;
  // const int N3 = 1000;

  const int TEST_COUNT = 256;
  {
    std::vector<Body1> bodies1(N1);
    Timer timer("Body1 Summary");

    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body1 Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i].position = bodies1[i].position + float3{1, 2, 3};
      }
    }
  }

  {
    std::vector<Body2> bodies1(N1);
    Timer timer("Body2 Summary");

    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body2 Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i].position = bodies1[i].position + bodies1[i].velocity * dt;
      }
    }
  }

  {
    std::vector<Body3> bodies1(N1);
    Timer timer("Body3 Summary");

    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body3 Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i].position = bodies1[i].position + bodies1[i].velocity * dt;
        // bodies1[i].velocity = bodies1[i].acceleration * dt;
        // bodies1[i].position = bodies1[i].position + bodies1[i].velocity * dt;
      }
    }
  }

  // test with array of pointers
  {
    std::vector<std::unique_ptr<Body1>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body1());
    }
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("Body1 Pointer Summary");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body1 Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i]->position = bodies1[i]->position + float3{1, 2, 3};
      }
    }
  }

  // test with array of pointers
  {
    std::vector<std::unique_ptr<Body2>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body2());
    }
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("Body2 Pointer Summary");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body2 Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i]->position = bodies1[i]->position + bodies1[i]->velocity * dt;
      }
    }
  }

  // test with array of pointers
  {
    std::vector<std::unique_ptr<Body3>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body3());
    }

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("Body3 Pointer Summary");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body3 Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i]->velocity = bodies1[i]->acceleration * dt;
        bodies1[i]->position = bodies1[i]->position + bodies1[i]->velocity * dt;
      }
    }
  }

  // test with array of pointers
  {
    std::vector<std::unique_ptr<IBody>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body1());
    }

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("IBody1 Pointer Summary");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("IBody Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i]->update(dt);
      }
    }
  }

  // test with array of pointers
  {
    std::vector<std::unique_ptr<IBody>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body2());
    }

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("IBody2 Pointer Summary");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("IBody Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i]->update(dt);
      }
    }
  }

  // test with array of pointers
  {
    std::vector<std::unique_ptr<IBody>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body3());
    }

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("IBody3 Pointer Summary");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("IBody Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i]->update(dt);
      }
    }
  }

  // test SoA
  {
    std::vector<float3> positionsStorage(N1);
    std::vector<float3> velocitiesStorage(N1);
    std::vector<float3> accelerationsStorage(N1);
    float3 *positions = positionsStorage.data();
    float3 *velocities = velocitiesStorage.data();
    const float3 * accelerations = accelerationsStorage.data();

    Timer timer("SoA Summary");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("SoA Iteration");
      for (int i = 0; i < N1; i++)
      {
        velocities[i] = accelerations[i] * dt;
        positions[i] = positions[i] + velocities[i] * dt;
      }
    }
  }

  printf("Hello, World!\n");
}