#include <iostream>
#include "ecs/ecs.h"
#include "timer.h"
#include "math_helper.h"

#include <vector>
#include <algorithm>
#include <random>
#include <array>

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
void main3(int TEST_COUNT, const int entityCount);

int main()
{
  // main2();
  // return 0;
  Timer timer("Main");
  // position
  // position, velocity
  // position, velocity, acceleration
  const float dt = 0.02f;
  const int N1 = 10;
  // const int N2 = 1000;
  // const int N3 = 1000;

  const int TEST_COUNT = 256;

  main3(TEST_COUNT, N1);

  {
    std::vector<Body1> bodies1(N1);
    Timer timer("Body1 Array");

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
    Timer timer("Body2 Array");

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
    Timer timer("Body3 Array");

    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body3 Iteration");
      for (int i = 0; i < N1; i++)
      {
        bodies1[i].velocity = bodies1[i].acceleration * dt;
        bodies1[i].position = bodies1[i].position + bodies1[i].velocity * dt;
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

    Timer timer("Body1 Pointers");
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

    Timer timer("Body2 Pointers");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body2 Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        Body2 *body = bodies1[i].get();
        body->position = body->position + body->velocity * dt;
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

    Timer timer("Body3 Pointer");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      // Timer timer("Body3 Pointer Iteration");
      for (int i = 0; i < N1; i++)
      {
        Body3 *body = bodies1[i].get();
        body->velocity = body->acceleration * dt;
        body->position = body->position + body->velocity * dt;
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
  {
    std::vector<std::unique_ptr<IBody>> bodies1(N1);
    for (int i = 0; i < N1; i++)
    {
      bodies1[i].reset(new Body2());
    }

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(bodies1), std::end(bodies1), rng);

    Timer timer("IBody2 Virtual");
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

    Timer timer("IBody3 Virtual");
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
    Timer timer("Body3 SoA Split Data");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      float3 * __restrict__ positions = positionsStorage.data();
      float3 * __restrict__ velocities  = velocitiesStorage.data();
      const float3 * accelerations = accelerationsStorage.data();

      // Timer timer("SoA Iteration");
      for (int i = 0; i < N1; i++)
      {
        *velocities = *accelerations * dt;
        *positions = *positions + *velocities * dt;

        positions += 1;
        velocities += 1;
        accelerations += 1;
      }
    }
  }
  // test SoA2
  {
    std::vector<float3> float3Storage(N1 * 3);
    Timer timer("Body3 SoA One Data");
    for (int j = 0 ; j < TEST_COUNT; j++)
    {
      float3 *__restrict__ positions = float3Storage.data() + 0;
      float3 *__restrict__ velocities = float3Storage.data() + N1;
      const float3 *__restrict__  accelerations = float3Storage.data() + N1 * 2;

      // Timer timer("SoA Iteration");
      for (int i = 0; i < N1; i++)
      {
        *velocities = *accelerations * dt;
        *positions = *positions + *velocities * dt;

        positions += 1;
        velocities += 1;
        accelerations += 1;
      }
    }
  }

}