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
  Collumn(ecs::ArchetypeChunkSize chunk_size_power, size_t size_of_element, size_t alignment_of_element, ecs::TypeId type_id) :
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

static_assert(sizeof(Collumn) == 56);
static_assert(alignof(Collumn) == 8);

} // namespace ecs
