#pragma once
#include "ecs/event.h"

namespace ecs
{

struct OnAppear { };
struct OnDisappear { };
struct OnTrack { };

ECS_EVENT_DECLARATION(OnAppear)
ECS_EVENT_DECLARATION(OnDisappear)
ECS_EVENT_DECLARATION(OnTrack)

} // namespace ecs
