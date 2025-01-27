#pragma once
#include "ecs/config.h"
#include "ecs/archetype_chunk_size.h"
#include "ecs/tiny_string.h"
#include <numeric> // for lcm

namespace ecs_details
{

struct Collumn
{
  std::vector<char *> chunks;
  ecs_details::tiny_string debugName;
  ecs::ComponentId componentId;
  uint32_t chunkSize;
  uint32_t sizeOfElement;
  ecs::TypeId typeId;
  uint32_t containerAlignment;
  Collumn(ecs::ArchetypeChunkSize chunk_size_power, size_t size_of_element, size_t alignment_of_element, ecs::TypeId type_id, const char *name, ecs::ComponentId component_id) :
    debugName(name),
    componentId(component_id),
    chunkSize(1 << chunk_size_power),
    sizeOfElement(size_of_element),
    typeId(type_id),
    containerAlignment(std::lcm(chunkSize, alignment_of_element))
  {}

  ~Collumn()
  {
    for (char *data : chunks)
    {
      operator delete[] (data, chunkSize * sizeOfElement, std::align_val_t{containerAlignment});
    }
  }

  void add_chunk()
  {
    chunks.push_back(new (std::align_val_t{containerAlignment}) char[chunkSize * sizeOfElement]);
  }

};

using TrackMask = uint32_t;
static const int MAX_TRACKED_COMPONENTS = 32;

struct TrackedCollumn final : public Collumn
{
  enum DirtyFlag : uint32_t
  {
    CLEAN = 0,
    DIRTY_ALL = 1 << 0,
    DIRTY_SOME = 1 << 1
  };
  std::vector<bool> dirtyState; // we assume that std::vector<bool> is a bitset
  int collumnIdx;
  uint32_t dirtyFlags = CLEAN;
  TrackedCollumn(ecs::ArchetypeChunkSize chunk_size_power, size_t size_of_element, size_t alignment_of_element, ecs::TypeId type_id, const char *name, ecs::ComponentId component_id, int collumn_idx) :
    Collumn(chunk_size_power, size_of_element, alignment_of_element, type_id, name, component_id), collumnIdx(collumn_idx)
  {}
  void mark_dirty()
  {
    dirtyFlags |= DIRTY_ALL;
  }
  void mark_dirty(uint32_t index)
  {
    dirtyState[index] = true;
    dirtyFlags |= DIRTY_SOME;
  }
  void reset_dirty()
  {
    dirtyFlags = CLEAN;
    dirtyState.assign(dirtyState.size(), false);
  }
};


// static_assert(sizeof(Collumn) == 56);
// static_assert(alignof(Collumn) == 8);

} // namespace ecs
