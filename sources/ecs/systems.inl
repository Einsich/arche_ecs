#include "ecs/ecs_manager.h"
#include "ecs/codegen_attributes.h"

#include "../../math_helper.h"

ECS_TYPE_DECLARATION(float3)
ECS_TYPE_DECLARATION(int)
ECS_TYPE_DECLARATION_ALIAS(std::string, "string")

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
