#pragma once

#include "ecs/config.h"
#include "ecs/component_init.h"

namespace ecs
{
  struct Template
  {
    ArchetypeId archetypeId;
    InitializerList args;
    std::string name;

    Template( const char *_name, InitializerList _args) : name(_name), args(_args) {}
  };
}