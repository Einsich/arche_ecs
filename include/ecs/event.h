#pragma once
#include "ecs/config.h"

namespace ecs
{
  using EventId = uint32_t;
  struct BaseEvent
  {

  };

  struct Event; // can be moved
  struct ImmediateEvent; // passed by const ref
  struct ReadbackEvent; // passed by read write ref

  struct EcsManager;

  void send_event(EcsManager &mgr, Event &&event);
  void send_event_immediate(EcsManager &mgr, const ImmediateEvent &event);
  void send_event_readback(EcsManager &mgr, ReadbackEvent &event);

  #define ECS_EVENT_DECLARE()
}