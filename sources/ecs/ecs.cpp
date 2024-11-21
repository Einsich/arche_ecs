#include "ecs/ecs.h"
#include "ecs/config.h"
#include "ecs/type_declaration.h"
#include "ecs/component_declaration.h"
#include "ecs/component_init.h"
#include "ecs/entity_id.h"
#include <assert.h>

#include <numeric>
#include <new>
namespace ecs
{

using Type = ska::flat_hash_map<ComponentId, TypeId  /*ComponentId already has TypeId*/>;
using ComponentList = std::vector<ComponentId>;

ArchetypeId get_archetype_id(const Type &type)
{
  uint32_t id = 0;
  for (const auto &[componentId, typeId]  : type)
  {
    id = fnv_hash(id, componentId);
  }
  return id;
}

enum ArchetypeChunkSize
{
  Few = 1,
  Medium = 4,
  Thousands = 10
};

// 1 << 14
// 1 << 10
// has space for 16 elements


struct Collumn
{
  std::string debugName;
  std::vector<char *> chunks;
  size_t capacity;
  size_t sizeOfElement;
  size_t alignmentOfElement;
  size_t chunkSize;
  size_t chunkMask;
  TypeId typeId;
  ArchetypeChunkSize chunkSizePower;
  std::align_val_t containerAlignment;

  Collumn(ArchetypeChunkSize chunk_size_power, size_t size_of_element, size_t alignment_of_element, TypeId type_id) :
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



struct Archetype
{
  Type type;
  ArchetypeId archetypeId;

  std::vector<Collumn> collumns;

  ska::flat_hash_map<ComponentId, size_t> componentToCollumnIndex;

  size_t entityCount = 0;

  Archetype() = default;
  Archetype(const TypeDeclarationMap &type_map, Type &&_type, ArchetypeChunkSize chunk_size_power) : type(std::move(_type))
  {
    archetypeId = get_archetype_id(type);
    collumns.reserve(type.size());
    componentToCollumnIndex.reserve(type.size());
    for (auto [componentId, typeId] : type)
    {
      const TypeDeclaration *typeDeclaration = type_map.find(typeId)->second.get();
      collumns.emplace_back(chunk_size_power, typeDeclaration->sizeOfElement, typeDeclaration->alignmentOfElement, typeId);
      Collumn &collumn = collumns.back();
      collumn.debugName = typeDeclaration->typeName;
      componentToCollumnIndex[componentId] = collumns.size() - 1;
    }
  }

  int get_component_index(ComponentId componentId) const
  {
    auto it = componentToCollumnIndex.find(componentId);
    return it != componentToCollumnIndex.end() ? it->second : -1;
  }

  // return index of the added entity
  uint32_t add_entity(const TypeDeclarationMap &type_map, InitializerList init_list)
  {
    for (ComponentInit &componentInit : init_list.args)
    {
      auto it = componentToCollumnIndex.find(componentInit.componentId);
      if (it == componentToCollumnIndex.end())
      {
        assert(false);
        continue;
      }
      Collumn &collumn = collumns[it->second];

      const TypeDeclaration *typeDeclaration = type_map.find(collumn.typeId)->second.get();
      if (entityCount == collumn.capacity)
      {
        collumn.add_chunk();
      }
      typeDeclaration->move_construct(collumn.get_data(entityCount), componentInit.data);
    }
    uint32_t entityIndex = entityCount;
    entityCount++;
    return entityIndex;
  }

  void remove_entity(const TypeDeclarationMap &type_map, uint32_t entityIndex)
  {
    for (Collumn &collumn : collumns)
    {
      const TypeDeclaration *typeDeclaration = type_map.find(collumn.typeId)->second.get();
      void *removedEntityComponentPtr = collumn.get_data(entityIndex);
      typeDeclaration->destruct(removedEntityComponentPtr);
      if (entityIndex != entityCount - 1)
      {
        void *lastEntityComponentPtr = collumn.get_data(entityCount - 1);
        typeDeclaration->move_construct(removedEntityComponentPtr, lastEntityComponentPtr);
      }
    }
    entityCount--;
  }
};

using ArchetypeMap = ska::flat_hash_map<ArchetypeId, Archetype>;



using ToComponentMap = std::vector<int>;

struct ArchetypeRecord
{
  ArchetypeId archetypeId;
  std::vector<int> toComponentIndex;
};

struct Query
{

  enum class ComponentAccess
  {
    READ_COPY,
    READ_ONLY,
    READ_WRITE,
    READ_ONLY_OPTIONAL,
    READ_WRITE_OPTIONAL,
    // REQUIRE,
    // EXCLUDE
  };

  struct ComponentAccessInfo
  {
    ComponentId componentId;
    ComponentAccess access;
  };

  std::string uniqueName;
  NameHash nameHash;
  std::vector<ComponentAccessInfo> querySignature;

  std::vector<ComponentId> requireComponents; // components without reading access
  std::vector<ComponentId> excludeComponents;

  bool try_registrate(const Archetype &archetype)
  {
    for (ComponentId componentId : requireComponents)
    {
      if (archetype.get_component_index(componentId) == -1)
      {
        return false;
      }
    }

    for (ComponentId componentId : excludeComponents)
    {
      if (archetype.get_component_index(componentId) != -1)
      {
        return false;
      }
    }

    ToComponentMap toComponentIndex;
    toComponentIndex.reserve(querySignature.size());
    for (const ComponentAccessInfo &componentAccessInfo : querySignature)
    {
      int componentIndex = archetype.get_component_index(componentAccessInfo.componentId);
      if (componentIndex == -1 && !(componentAccessInfo.access == ComponentAccess::READ_ONLY_OPTIONAL || componentAccessInfo.access == ComponentAccess::READ_WRITE_OPTIONAL))
      {
        return false;
      }
      toComponentIndex.push_back(componentIndex);
    }

    archetypesCache.push_back({archetype.archetypeId, std::move(toComponentIndex)});

    return true;
  }

  std::vector<ArchetypeRecord> archetypesCache;
  void (*update_archetype)(Archetype &archetype, const ToComponentMap &to_archetype_component);
};

struct EcsManager
{
  TypeDeclarationMap typeMap;
  ComponentDeclarationMap componentMap;
  ska::flat_hash_map<ArchetypeId, Archetype> archetypeMap;
  ska::flat_hash_map<uint32_t, Query> queries;
  EntityContainer entityContainer;

  ecs::TypeId EntityIdTypeId;
  ecs::ComponentId eidComponentId;

  EcsManager()
  {
    EntityIdTypeId = ecs::type_registration<ecs::EntityId>(typeMap, "EntityId");
    eidComponentId = ecs::component_registration(componentMap, EntityIdTypeId, "eid");
  }
  ecs::EntityId create_entity(ArchetypeId archetypeId, InitializerList init_list)
  {
    auto it = archetypeMap.find(archetypeId);
    if (it == archetypeMap.end())
    {
      printf("Archetype not found\n");
      return EntityId();
    }
    Archetype &archetype = it->second;
    uint32_t entityIndex = archetype.entityCount;
    ecs::EntityId eid = entityContainer.create_entity(archetypeId, entityIndex);
    init_list.push_back(ecs::ComponentInit(eidComponentId, ecs::EntityId(eid)));
    assert(archetype.type.size() == init_list.size());
    archetype.add_entity(typeMap, std::move(init_list));

    return eid;
  }

  bool destroy_entity(ecs::EntityId eid)
  {
    ecs::ArchetypeId archetypeId;
    uint32_t componentIndex;
    if (entityContainer.get(eid, archetypeId, componentIndex))
    {
      auto it = archetypeMap.find(archetypeId);
      if (it == archetypeMap.end())
      {
        printf("Archetype not found\n");
        return false;
      }
      Archetype &archetype = it->second;
      archetype.remove_entity(typeMap, componentIndex);
      entityContainer.destroy_entity(eid);
      return true;
    }
    return false;
  }
};

// delete
ArchetypeId archetype_registration(EcsManager &manager, ComponentList &&components, ArchetypeChunkSize chunk_size_power)
{
  Type type;
  type.reserve(components.size());
  for (ComponentId componentId : components)
  {
    type[componentId] = manager.componentMap.find(componentId)->second->typeId;
  }
  type[manager.eidComponentId] = manager.EntityIdTypeId;

  ArchetypeId archetypeId = get_archetype_id(type);
  if (manager.archetypeMap.find(archetypeId) != manager.archetypeMap.end())
  {
    printf("Archetype already exists\n");
    return archetypeId;
  }
  manager.archetypeMap[archetypeId] = Archetype(manager.typeMap, std::move(type), chunk_size_power);
  return archetypeId;
}

struct Template
{
  ArchetypeId archetypeId;
  InitializerList args;
  std::string name;

  Template(EcsManager &mgr, const char *_name, InitializerList _args) : name(_name), args(_args)
  {
    auto it = mgr.archetypeMap.find(mgr.find_template(name));
    if (it == mgr.archetypeMap.end())
    {
      printf("Template not found\n");
      return;
    }
    archetypeId = it->first;

  }
};

TemplateId template_registration(EcsManager &manager, const char *_name, InitializerList &&components, ArchetypeChunkSize chunk_size_power)
{
  Type type;
  type.reserve(components.size());
  for (const ComponentInit &component : components.args)
  {
    type[component.componentId] = manager.componentMap.find(component.componentId)->second->typeId;
  }
  type[manager.eidComponentId] = manager.EntityIdTypeId;

  ArchetypeId archetypeId = get_archetype_id(type);

  if (manager.archetypeMap.find(archetypeId) == manager.archetypeMap.end())
  {
    manager.archetypeMap[archetypeId] = Archetype(manager.typeMap, std::move(type), chunk_size_power);
  }

}

void update_query(ArchetypeMap &archetype_map, const Query &query)
{
  for (const auto &archetypeRecord : query.archetypesCache)
  {
    auto it = archetype_map.find(archetypeRecord.archetypeId);
    if (it == archetype_map.end())
    {
      continue;
    }
    query.update_archetype(it->second, archetypeRecord.toComponentIndex);
  }
}
}

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#define UNUSED(x) (void)(x)

#include "../../math_helper.h"

// user_code.cpp.inl

void update(ecs::EntityId eid, float3 &position, const float3 &velocity)
{
  printf("update [%d/%d] (%f %f %f), (%f %f %f)\n", eid.entityIndex, eid.generation, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
}


// user_code.cpp.gen
// #include "user_code.cpp.inl"

void update_loop_codegen(char *eid, char *position, char *velocity, size_t elementCount)
{
  for (size_t i = 0; i < elementCount; i++)
  {
    update(*(ecs::EntityId *)(eid), *(float3 *)(position), *(const float3 *)(velocity));
    eid += sizeof(ecs::EntityId);
    position += sizeof(float3);
    velocity += sizeof(float3);
  }
}

void update_archetype_codegen(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  ecs::Collumn &eidCollumn = archetype.collumns[to_archetype_component[0]];
  ecs::Collumn &positionCollumn = archetype.collumns[to_archetype_component[1]];
  ecs::Collumn &velocityCollumn = archetype.collumns[to_archetype_component[2]];
  for (uint32_t chunkIdx = 0, chunkCount = eidCollumn.chunks.size(); chunkIdx < chunkCount; chunkIdx++)
  {
    uint32_t elementCount = std::min(archetype.entityCount - eidCollumn.chunkSize * chunkIdx, eidCollumn.chunkSize);

    char *eid = eidCollumn.chunks[chunkIdx];
    char *position = positionCollumn.chunks[chunkIdx];
    char *velocity = velocityCollumn.chunks[chunkIdx];
    update_loop_codegen(eid, position, velocity, elementCount);
  }
}


void print_name(const std::string &name, float3 position)
{
  printf("print_name [%s] (%f %f %f)\n", name.c_str(), position.x, position.y, position.z);
}


// user_code.cpp.gen
// #include "user_code.cpp.inl"

void print_name_loop_codegen(char *name, char *position, size_t elementCount)
{
  for (size_t i = 0; i < elementCount; i++)
  {
    print_name(*(std::string *)(name), *(float3 *)(position));
    name += sizeof(std::string);
    position += sizeof(float3);
  }
}

void print_name_archetype_codegen(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  ecs::Collumn &nameCollumn = archetype.collumns[to_archetype_component[0]];
  ecs::Collumn &positionCollumn = archetype.collumns[to_archetype_component[1]];
  for (uint32_t chunkIdx = 0, chunkCount = nameCollumn.chunks.size(); chunkIdx < chunkCount; chunkIdx++)
  {
    uint32_t elementCount = std::min(archetype.entityCount - nameCollumn.chunkSize * chunkIdx, nameCollumn.chunkSize);

    char *name = nameCollumn.chunks[chunkIdx];
    char *position = positionCollumn.chunks[chunkIdx];
    print_name_loop_codegen(name, position, elementCount);
  }
}

int main2()
{
  const bool EntityContainerTest = false;
  if (EntityContainerTest)
  {
    ecs::EntityContainer entityContainer;
    ecs::EntityId entityId = entityContainer.create_entity(0, 0);
    assert(entityContainer.is_alive(entityId));
    entityContainer.destroy_entity(entityId);
    assert(!entityContainer.is_alive(entityId));
    assert(!entityContainer.is_alive(ecs::EntityId()));

    ecs::EntityId entityId1 = entityContainer.create_entity(0, 0);
    assert(entityContainer.is_alive(entityId1));
    assert(entityId != entityId1);
    UNUSED(entityId);
    UNUSED(entityId1);
  }
  ecs::EcsManager mgr;

  ecs::TypeId float3Id = ecs::type_registration<float3>(mgr.typeMap, "float3");
  ecs::TypeId intId = ecs::type_registration<int>(mgr.typeMap, "int");
  ecs::TypeId stringId = ecs::type_registration<std::string>(mgr.typeMap, "string");

  for (const auto &[id, type] : mgr.typeMap)
  {
    printf("[ECS] type: %s, typeId: %x\n", type->typeName.c_str(), type->typeId);
  }

  ecs::ComponentId positionId = ecs::component_registration(mgr.componentMap, ecs::hash("float3")/* float3Id */, "position");
  ecs::ComponentId velocityId = ecs::component_registration(mgr.componentMap, float3Id, "velocity");
  ecs::ComponentId healthId = ecs::component_registration(mgr.componentMap, intId, "health");
  ecs::ComponentId nameId = ecs::component_registration(mgr.componentMap, stringId, "name");

  for (const auto &[id, component] : mgr.componentMap)
  {
    printf("[ECS] component: %s, componentId: %x, typeId: %x\n", component->name.c_str(), component->componentId, component->typeId);
  }

  ecs::ArchetypeId archetype1 = archetype_registration(mgr, {positionId, velocityId}, ecs::ArchetypeChunkSize::Medium);
  ecs::ArchetypeId archetype2 = archetype_registration(mgr, {velocityId, positionId, nameId}, ecs::ArchetypeChunkSize::Medium);
  ecs::ArchetypeId archetype3 = archetype_registration(mgr, {velocityId, positionId, healthId, nameId}, ecs::ArchetypeChunkSize::Medium);

  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    printf("[ECS] archetype: %x, components: %zu\n", archetype.archetypeId, archetype.collumns.size());
  }


  ecs::Query update_position_query;
  ecs::Query print_name_query;

  {
    ecs::Query query;
    query.uniqueName = __FILE__ LINE_STRING "update_position";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature = {
      {mgr.eidComponentId, ecs::Query::ComponentAccess::READ_COPY},
      {positionId, ecs::Query::ComponentAccess::READ_WRITE},
      {velocityId, ecs::Query::ComponentAccess::READ_ONLY}
    };
    query.requireComponents = {};
    query.excludeComponents = {healthId};
    query.update_archetype = update_archetype_codegen;

    for (auto &[id, archetype]  : mgr.archetypeMap)
    {
      query.try_registrate(archetype);
    }
    update_position_query = std::move(query);
  }
  {
    ecs::Query query;
    query.uniqueName = __FILE__ LINE_STRING "print_name";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature = {
      {nameId, ecs::Query::ComponentAccess::READ_ONLY},
      {positionId, ecs::Query::ComponentAccess::READ_COPY}
    };
    query.requireComponents = {};
    query.excludeComponents = {};
    query.update_archetype = print_name_archetype_codegen;
    for (auto &[id, archetype]  : mgr.archetypeMap)
    {
      query.try_registrate(archetype);
    }
    print_name_query = std::move(query);
  }

  //unfortunately, we can't use initializer_list here, because it doesn't support move semantics
  // _CONSTEXPR20 vector(initializer_list<_Ty> _Ilist, const _Alloc& _Al = _Alloc())

  ecs::InitializerList args;
  args.push_back(ecs::ComponentInit{positionId, float3{1, 2, 3}});
  args.push_back(ecs::ComponentInit{velocityId, float3{4, 5, 6}});
  mgr.create_entity(archetype1, std::move(args));


  mgr.create_entity(
    archetype3,
    ecs::InitializerList{
      {
        ecs::ComponentInit{positionId, float3{7, 8, 9}},
        ecs::ComponentInit{velocityId, float3{10, 11, 12}},
        ecs::ComponentInit{healthId, 100},
        ecs::ComponentInit{nameId, std::string("entity1")}
      }
    }
  );

  auto movableTemplate = ecs::Template{
      "movable",
      ecs::ComponentInit{positionId, float3{0, 0, 0}},
      ecs::ComponentInit{velocityId, float3{0, 0, 0}}
  }
  auto brickTemplate = movableTemplate + ecs::Template{
    {
      "brick",
      ecs::ComponentInit{healthId, 10},
      ecs::ComponentInit{nameId, std::string("brick")}
    }
  }
  auto humanTemplate = movableTemplate + ecs::Template{
    {
      "human",
      ecs::ComponentInit{healthId, 50},
      ecs::ComponentInit{nameId, std::string("human")}
    }
  }
  auto bigBrickTemplate = brickTemplate + ecs::Template{
    {
      "bigbrick",
      ecs::ComponentInit{healthId, 100},
    }
  }
  auto templateId = mgr.find_template("movable");
  ecs::EntityId eid1 = mgr.create_entity(
    templateId , //"movable" // ,
    {{
      {positionId, float3{13, 14, 15}},
      {velocityId, float3{16, 17, 18}},
      {nameId, std::string("entity1")}
    }}
  );

  for (int i = 0; i < 100; i++)
  {
    float f = i;
    ecs::EntityId eid = mgr.create_entity(
      archetype2,
      {{
        {positionId, float3{f, f, f}},
        // {velocityId, float3{f, f, f}},
        {nameId, std::string("very_long_node_name############") + std::to_string(i)}
      }}
    );
    if (i < 15)
      assert(mgr.destroy_entity(eid));
  }

  assert(mgr.destroy_entity(eid1));

  ecs::update_query(mgr.archetypeMap, update_position_query);
  ecs::update_query(mgr.archetypeMap, print_name_query);

  return 0;
}