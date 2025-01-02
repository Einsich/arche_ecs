#pragma once
#include "ecs/config.h"

namespace ecs
{
  template<typename T>
  struct EventInfo;

  struct Event
  {
  private:
    EventId eventId;
    const void *ptr;
  public:
    Event(EventId event_id, const void *event_ptr) : eventId(event_id), ptr(event_ptr) {}

    template <typename T>
    const T *cast() const
    {
      return eventId == EventInfo<T>::eventId ? static_cast<const T *>(ptr) : nullptr;
    }

    template <typename T>
    bool is() const
    {
      return eventId == EventInfo<T>::eventId;
    }
  };


  #define ECS_EVENT_DECLARATION_VERBOSE(CPP_TYPE, STRING_ALIAS, IS_TRIVIALLY_RELOCATABLE) \
  template<> \
  struct ecs::EventInfo<CPP_TYPE> \
  { \
    static constexpr ecs::EventId eventId = ecs::hash(STRING_ALIAS); \
    static constexpr const char *typeName = STRING_ALIAS; \
    static constexpr bool isTriviallyRelocatable = IS_TRIVIALLY_RELOCATABLE; \
  };

#define ECS_EVENT_DECLARATION(CPP_TYPE) ECS_EVENT_DECLARATION_VERBOSE(CPP_TYPE, #CPP_TYPE, true)

}