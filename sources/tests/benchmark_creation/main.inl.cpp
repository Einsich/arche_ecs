#include <ecs/codegen_helpers.h>
#include "main.inl"
//Code-generator production

static void ecs_registration(ecs::EcsManager &mgr)
{
  ECS_UNUSED(mgr);
}
static ecs_details::CodegenFileRegistration fileRegistration(&ecs_registration);
ECS_PULL_DEFINITION(variable_pull_main)
