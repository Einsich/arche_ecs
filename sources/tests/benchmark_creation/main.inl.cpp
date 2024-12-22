#include "main.inl"
#include <ecs/query_iteration.h>
//Code-generator production

static void ecs_registration(ecs::EcsManager &mgr)
{
}
static ecs::CodegenFileRegistration fileRegistration(&ecs_registration);
ECS_PULL_DEFINITION(variable_pull_main)
