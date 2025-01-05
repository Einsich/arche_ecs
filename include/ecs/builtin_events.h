#pragma once
#include "ecs/event.h"

namespace ecs
{

struct OnAppear { };
struct OnDisappear { };

ECS_EVENT_DECLARATION(OnAppear)
ECS_EVENT_DECLARATION(OnDisappear)

} // namespace ecs
