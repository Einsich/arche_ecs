#include "ecs/ecs.h"
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "ecs/ska/flat_hash_map.hpp"

using ComponentId = uint32_t;
using NameHash = uint32_t;

template <typename T>
void construct_default(void *data)
{
  new (data) T();
}

template <typename T>
void destruct(void *data)
{
  ((T *)data)->~T();
}

template <typename T>
void copy_construct(void *dest, const void *src)
{
  new (dest) T(*(T *)src);
}

template <typename T>
void move_construct(void *dest, void *src)
{
  new (dest) T(std::move(*(T *)src));
}
struct TypeDeclaration
{
  std::string typeName;
  uint32_t sizeOfElement;
  uint32_t alignmentOfElement;

  void (*construct_default)(void *data);
  void (*destruct)(void *data);
  void (*copy_construct)(void *dest, const void *src);
  void (*move_construct)(void *dest, void *src);
  // component.construct_default = construct_default<ComponentType>;
};

using TypeDeclarationMap = ska::flat_hash_map<std::string, std::unique_ptr<TypeDeclaration>>;

const TypeDeclaration *find(const TypeDeclarationMap &type_map, const char *type_name)
{
  const auto it = type_map.find_as(type_name);
  return it != type_map.end() ? it->second.get() : nullptr;
}

void add_type_declaration(TypeDeclarationMap &type_map, std::unique_ptr<TypeDeclaration> type_declaration)
{
  type_map[type_declaration->typeName] = std::move(type_declaration);
}

struct Component
{
  const TypeDeclaration *typeDeclaration;
  std::string name;
};



using Type = std::vector<ComponentId>;


size_t component_elemet_size(ComponentId id)
{
  //
  return 0;
}

ComponentId find_component_id(const char *type_name, const char *component_name)
{
  return 0;
}

struct Collumn
{
  char *data;
  size_t elementCount, elementCapacity, sizeOfElement;

  Collumn(size_t elementCount, size_t sizeOfElement) :
    data(new char[elementCapacity * sizeOfElement]),
    elementCount(0),
    elementCapacity(elementCapacity),
    sizeOfElement(sizeOfElement) {}

  ~Collumn()
  {
    delete[] data;
  }
};

struct Archetype
{
  Type type;

  std::vector<Collumn> collumns;

  ska::flat_hash_map<ComponentId, size_t> componentToCollumnIndex;

  Archetype(const Type &type, size_t elementCount) : type(type)
  {
    collumns.reserve(type.size());
    componentToCollumnIndex.reserve(type.size());
    for (auto componentId : type)
    {
      collumns.emplace_back(elementCount, component_elemet_size(componentId));
      componentToCollumnIndex[componentId] = collumns.size() - 1;
    }
  }
};

#include "../../math_helper.h"

using ToComponentMap = std::vector<int>;

struct ArchetypeRecord
{
  Archetype *archetype;
  std::vector<int> toComponentIndex;
};

struct Query
{
  Type type;
  bool match(const Archetype &archetype);

  std::vector<ArchetypeRecord> archetypes;
  void (*update_archetype)(Archetype &archetype, const ToComponentMap &to_archetype_component);
};

struct EcsManager
{
  TypeDeclarationMap typeMap;
  std::vector<Archetype> archetypes;
  ska::flat_hash_map<uint32_t, Query> queries;
};

void update(float3 &position, const float3 &velocity);

//generate mangled name for update(float3&, float3 const&)
// "_Z6updateR6float3RK6float3" + __FILE__ + __LINE__


void update(char *position, char *velocity, size_t elementCount)
{
  for (size_t i = 0; i < elementCount; i++)
  {
    update(*(float3 *)(position), *(float3 *)(position));
    position += sizeof(float3);
    velocity += sizeof(float3);
  }
}

void update_archetype(Archetype &archetype, const ToComponentMap &to_archetype_component)
{
  Collumn &positionCollumn = archetype.collumns[to_archetype_component[0]];
  Collumn &velocityCollumn = archetype.collumns[to_archetype_component[1]];
  update(positionCollumn.data, velocityCollumn.data, positionCollumn.elementCount);
}

void update_query(const Query &query)
{
  for (const auto &archetypeRecord : query.archetypes)
  {
    query.update_archetype(*archetypeRecord.archetype, archetypeRecord.toComponentIndex);
  }
}