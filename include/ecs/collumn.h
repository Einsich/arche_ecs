#pragma once
#include "ecs/config.h"
#include "ecs/archetype_chunk_size.h"
#include <numeric> // for lcm

namespace ecs_details
{

struct Collumn
{
  std::string debugName;
  std::vector<char *> chunks;
  size_t capacity;
  size_t sizeOfElement;
  size_t alignmentOfElement;
  size_t chunkSize;
  size_t chunkMask;
  ecs::TypeId typeId;
  ecs::ArchetypeChunkSize chunkSizePower;
  std::align_val_t containerAlignment;
  ecs::ComponentId componentId;

  Collumn(ecs::ArchetypeChunkSize chunk_size_power, size_t size_of_element, size_t alignment_of_element, ecs::TypeId type_id) :
    capacity(0),
    sizeOfElement(size_of_element),
    alignmentOfElement(alignment_of_element),
    chunkSize(1 << chunk_size_power),
    chunkMask(chunkSize - 1),
    typeId(type_id),
    chunkSizePower(chunk_size_power),
    containerAlignment(std::align_val_t{std::lcm(chunkSize, alignmentOfElement)})
  {}

  ~Collumn()
  {
    for (char *data : chunks)
    {
      operator delete[] (data, chunkSize * sizeOfElement, containerAlignment);
    }
  }

  void add_chunk()
  {
    capacity += chunkSize;
    chunks.push_back(new (containerAlignment) char[chunkSize * sizeOfElement]);
  }

  char *get_data(uint32_t linear_index)
  {
    return chunks[linear_index >> chunkSizePower] + (linear_index & chunkMask) * sizeOfElement;
  }
};

} // namespace ecs
