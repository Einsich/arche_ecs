#include <ecs/query_iteration.h>
#include "main.inl"
//Code-generator production

static void ecs_registration(ecs::EcsManager &mgr)
{
}
static ecs::CodegenFileRegistration fileRegistration(&ecs_registration);
ECS_PULL_DEFINITION(variable_pull_main)
