#include <iostream>
#include "ecs/ecs.h"
#include "timer.h"
#include "math_helper.h"

#include <vector>
#include <algorithm>
#include <random>
#include <array>

float dt = 0.02f;
struct IBody
{
  std::array<float, 128 * 2> data;
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
// static_assert(sizeof(Body3) == 48);

int main2();
void main3(int TEST_COUNT, const int entityCount);

int main()
{
  // main2();
  // return 0;
  // Timer timer("Main");
  // position
  // position, velocity
  // position, velocity, acceleration

  int hot_cache_rate = 7; // [0, 8]
  int N1 = 12 * (1 << (8 - hot_cache_rate));
  // const int N2 = 1000;
  // const int N3 = 1000;

  int TEST_COUNT = (1 << (hot_cache_rate));

  const bool Body1ArrayTest = true;
  const bool Body2ArrayTest = true;
  const bool Body3ArrayTest = true;
  const bool Body1PointerTest = false;
  const bool Body2PointerTest = false;
  const bool Body3PointerTest = false;
  const bool IBody1VirtualTest = false;
  const bool IBody2VirtualTest = false;
  const bool IBody3VirtualTest = false;
  const bool Body3SoASplitDataTest = true;
  const bool Body3SoAOneDataTest = true;

  if (Body1ArrayTest)
  {
    // Timer timerCreation("Body1 Array Creation");
    std::vector<Body1> bodies1(N1);
    // timerCreation.stop();
    Timer timer("Body1 Array");

    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      for (int i = 0; i < N1; i++)
      {
        update1(bodies1[i].position);
      }
    }
  }

  if (Body2ArrayTest)
  {
    // Timer timerCreation("Body2 Array Creation");
    std::vector<Body2> bodies1(N1);
    // timerCreation.stop();
    Timer timer("Body2 Array");

    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body2 Iteration");
      for (int i = 0; i < N1; i++)
      {
        update2(bodies1[i].position, bodies1[i].velocity);
      }
    }
  }

  if (Body3ArrayTest)
  {
    // Timer timerCreation("Body3 Array Creation");
    std::vector<Body3> bodies1(N1);
    // timerCreation.stop();
    Timer timer("Body3 Array");

    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body3 Iteration");
      for (int i = 0; i < N1; i++)
      {
        update3(bodies1[i].position, bodies1[i].velocity, bodies1[i].acceleration);
      }
    }
  }

  // test with array of pointers
  if (Body1PointerTest)
  {
    Timer timerCreation("Body1 Pointers Creation");
    std::vector<std::unique_ptr<Body1>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body1());
    }
    timerCreation.stop();
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("Body1 Pointers");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body1 Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        update1(bodies1[i]->position);
      }
    }
  }

  // test with array of pointers
  if (Body2PointerTest)
  {
    Timer timerCreation("Body2 Pointers Creation");
    std::vector<std::unique_ptr<Body2>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body2());
    }
    timerCreation.stop();
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("Body2 Pointers");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body2 Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        Body2 *body = bodies1[i].get();
        update2(body->position, body->velocity);
      }
    }
  }

  // test with array of pointers
  if (Body3PointerTest)
  {
    Timer timerCreation("Body3 Pointers Creation");
    std::vector<std::unique_ptr<Body3>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body3());
    }
    timerCreation.stop();

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("Body3 Pointer");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body3 Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        Body3 *body = bodies1[i].get();
        update3(body->position, body->velocity, body->acceleration);
      }
    }
  }

  // test with array of pointers
  if (IBody1VirtualTest)
  {
    Timer timerCreation("IBody1 Virtual Creation");
    std::vector<std::unique_ptr<IBody>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body1());
    }
    timerCreation.stop();

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("IBody1 Virtual");
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
  if (IBody2VirtualTest)
  {
    Timer timerCreation("IBody2 Virtual Creation");
    std::vector<std::unique_ptr<IBody>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body2());
    }
    timerCreation.stop();

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("IBody2 Virtual");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      for (int i = 0; i < N1; i++)
      {
        bodies1[i]->update(dt);
      }
    }
  }

  // test with array of pointers
  if (IBody3VirtualTest)
  {
    Timer timerCreation("IBody2 Virtual Creation");
    std::vector<std::unique_ptr<IBody>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body3());
    }
    timerCreation.stop();

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("IBody3 Virtual");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      for (int i = 0; i < N1; i++)
      {
        bodies1[i]->update(dt);
      }
    }
  }

  // test SoA
  if (Body3SoASplitDataTest)
  {
    Timer timerCreation("Body3 SoA Split Creation");
    std::vector<float3> positionsStorage(N1);
    std::vector<float3> velocitiesStorage(N1);
    std::vector<float3> accelerationsStorage(N1);
    timerCreation.stop();

    Timer timer("Body3 SoA Split Data");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      float3 * __restrict__ positions = positionsStorage.data();
      float3 * __restrict__ velocities  = velocitiesStorage.data();
      const float3 * __restrict__ accelerations = accelerationsStorage.data();

      for (int i = 0; i < N1; i++)
      {
        update3(*positions, *velocities, *accelerations);

        positions += 1;
        velocities += 1;
        accelerations += 1;
      }
    }
  }
  // test SoA2
  if (Body3SoAOneDataTest)
  {
    Timer timerCreation("Body3 SoA One Creation");
    std::vector<float3> float3Storage(N1 * 3);
    timerCreation.stop();
    Timer timer("Body3 SoA One Data");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      float3 * __restrict__ positions = float3Storage.data() + 0;
      float3 * __restrict__ velocities = float3Storage.data() + N1;
      const float3 * __restrict__ accelerations = float3Storage.data() + N1 * 2;

      for (int i = 0; i < N1; i++)
      {
        update3(*positions, *velocities, *accelerations);
        positions += 1;
        velocities += 1;
        accelerations += 1;
      }
    }
  }
  main3(TEST_COUNT, N1);

}