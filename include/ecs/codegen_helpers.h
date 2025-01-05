#pragma once
#include "ecs/ecs_manager.h"
#include <ecs/iteration_helpers.h>

namespace ecs_details
{

#define ECS_PULL_DECLARATION(PULL_VAR) extern size_t PULL_VAR;
#define ECS_PULL_DEFINITION(PULL_VAR) size_t PULL_VAR = (uintptr_t)&PULL_VAR;

struct CodegenFileRegistration
{
  using RegistrationFunction = void (*)(ecs::EcsManager &mgr);
  RegistrationFunction registrationFunction;
  static inline CodegenFileRegistration *tail = nullptr;
  const CodegenFileRegistration *next = nullptr;

  CodegenFileRegistration(RegistrationFunction registrationFunction) : registrationFunction(registrationFunction)
  {
    next = tail;
    tail = this;
  }

};

} // namespace ecs_details