#pragma once


#define ECS_SYSTEM(...) static void
#define ECS_QUERY(...)
#define ECS_EVENT(...) static void
#define ECS_REQUEST(...) static void

#define ECS_PULL_DECLARATION(PULL_VAR) extern size_t PULL_VAR;
#define ECS_PULL_DEFINITION(PULL_VAR) size_t PULL_VAR = (uintptr_t)&PULL_VAR;

namespace ecs
{
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

  static void register_all_codegen_files(ecs::EcsManager &mgr)
  {
    for (const CodegenFileRegistration *info = tail; info; info = info->next)
    {
      info->registrationFunction(mgr);
    }
  }
};

} // namespace ecs