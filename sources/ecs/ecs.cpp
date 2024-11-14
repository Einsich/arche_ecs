#include "ecs/ecs.h"
#include "ecs/config.h"
#include "ecs/type_declaration.h"
#include "ecs/component_declaration.h"
#include <assert.h>
#include <any>
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


struct Collumn
{
  char *data;
  size_t elementCount, elementCapacity, sizeOfElement;
  TypeId typeId;

  Collumn(size_t element_capacity, size_t size_of_element, TypeId type_id) :
    data(new char[element_capacity * size_of_element]),
    elementCount(0),
    elementCapacity(element_capacity),
    sizeOfElement(size_of_element),
    typeId(type_id) {}

  ~Collumn()
  {
    delete[] data;
  }
};


template <typename T>
void ComponentInitDestruct(void *data)
{
  if (data)
  {
    ((T *)data)->~T();
    delete (T *)data;
  }
}

struct ComponentInit
{
  ComponentId componentId;
  void *data;

  void (*destructor)(void *data);

  template<typename T>
  ComponentInit(ComponentId _componentId, T &&_data) : componentId(_componentId), data(new T(std::forward<T>(_data))),
  destructor(ComponentInitDestruct<T>) {}

  ComponentInit(ComponentInit &&other) : componentId(other.componentId), data(other.data), destructor(other.destructor)
  {
    other.data = nullptr;
  }
  ComponentInit(const ComponentInit &other) = delete;
  ComponentInit &operator=(const ComponentInit &other) = delete;
  ~ComponentInit()
  {
    destructor(data);
  }
};

struct Archetype
{
  Type type;
  ArchetypeId archetypeId;

  std::vector<Collumn> collumns;

  ska::flat_hash_map<ComponentId, size_t> componentToCollumnIndex;

  Archetype() = default;
  Archetype(const TypeDeclarationMap &type_map, Type &&_type, size_t elementCount) : type(std::move(_type))
  {
    archetypeId = get_archetype_id(type);
    collumns.reserve(type.size());
    componentToCollumnIndex.reserve(type.size());
    for (auto [componentId, typeId] : type)
    {
      const TypeDeclaration *typeDeclaration = type_map.find(typeId)->second.get();
      collumns.emplace_back(elementCount, typeDeclaration->sizeOfElement, typeId);
      componentToCollumnIndex[componentId] = collumns.size() - 1;
    }
  }

  int get_component_index(ComponentId componentId) const
  {
    auto it = componentToCollumnIndex.find(componentId);
    return it != componentToCollumnIndex.end() ? it->second : -1;
  }

  void add_entity(const TypeDeclarationMap &type_map, std::vector<ComponentInit> args)
  {
    for (ComponentInit &componentInit : args)
    {
      auto it = componentToCollumnIndex.find(componentInit.componentId);
      if (it == componentToCollumnIndex.end())
      {
        assert(false);
        continue;
      }
      Collumn &collumn = collumns[it->second];

      const TypeDeclaration *typeDeclaration = type_map.find(collumn.typeId)->second.get();
      if (collumn.elementCount == collumn.elementCapacity)
      {
        printf("Collumn is full\n");
        continue;
      }
      typeDeclaration->move_construct(collumn.data + collumn.elementCount, componentInit.data);
      collumn.elementCount++;
    }
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


  void create_entity(ArchetypeId archetypeId, std::vector<ComponentInit> args)
  {
    auto it = archetypeMap.find(archetypeId);
    if (it == archetypeMap.end())
    {
      printf("Archetype not found\n");
      return;
    }
    Archetype &archetype = it->second;
    assert(archetype.type.size() == args.size());
    archetype.add_entity(typeMap, std::move(args));
  }
};

ArchetypeId archetype_registration(EcsManager &manager, ComponentList &&components, size_t elementCount)
{
  Type type;
  type.reserve(components.size());
  for (ComponentId componentId : components)
  {
    type[componentId] = manager.componentMap.find(componentId)->second->typeId;
  }
  ArchetypeId archetypeId = get_archetype_id(type);
  if (manager.archetypeMap.find(archetypeId) != manager.archetypeMap.end())
  {
    printf("Archetype already exists\n");
    return archetypeId;
  }
  manager.archetypeMap[archetypeId] = Archetype(manager.typeMap, std::move(type), elementCount);
  return archetypeId;
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

void update(float3 &position, const float3 &velocity)
{
  printf("update (%f %f %f), (%f %f %f)\n", position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
}

//generate mangled name for update(float3&, float3 const&)
// "_Z6updateR6float3RK6float3" + __FILE__ + __LINE__

// user_code.cpp.gen
// #include "user_code.cpp.inl"

void update_loop_codegen(char *position, char *velocity, size_t elementCount)
{
  for (size_t i = 0; i < elementCount; i++)
  {
    update(*(float3 *)(position), *(const float3 *)(velocity));
    position += sizeof(float3);
    velocity += sizeof(float3);
  }
}

void update_archetype_codegen(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  ecs::Collumn &positionCollumn = archetype.collumns[to_archetype_component[0]];
  ecs::Collumn &velocityCollumn = archetype.collumns[to_archetype_component[1]];
  update_loop_codegen(positionCollumn.data, velocityCollumn.data, positionCollumn.elementCount);
}

int main2()
{
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

  ecs::ArchetypeId archetype1 = archetype_registration(mgr, {positionId, velocityId}, 1000);
  ecs::ArchetypeId archetype2 = archetype_registration(mgr, {velocityId, positionId, healthId}, 1000);
  ecs::ArchetypeId archetype3 = archetype_registration(mgr, {velocityId, positionId, healthId, nameId}, 1000);

  for (const auto &[id, archetype] : mgr.archetypeMap)
  {
    printf("[ECS] archetype: %x, components: %zu\n", archetype.archetypeId, archetype.collumns.size());
  }



  ecs::Query query;
  query.uniqueName = __FILE__ LINE_STRING "update_position";
  query.nameHash = ecs::hash(query.uniqueName.c_str());
  query.querySignature = {
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

  ecs::update_query(mgr.archetypeMap, query);


  mgr.create_entity(
    archetype1,
    {ecs::ComponentInit{positionId, float3{1, 2, 3}},
    ecs::ComponentInit{velocityId, float3{4, 5, 6}}}
  );


  mgr.create_entity(
    archetype3,
    {ecs::ComponentInit{positionId, float3{7, 8, 9}},
    ecs::ComponentInit{velocityId, float3{10, 11, 12}},
    ecs::ComponentInit{healthId, 100},
    ecs::ComponentInit{nameId, std::string("entity1")}}
  );

  mgr.create_entity(
    archetype2,
    {ecs::ComponentInit{positionId, float3{13, 14, 15}},
    ecs::ComponentInit{velocityId, float3{16, 17, 18}},
    ecs::ComponentInit{nameId, std::string("entity1")}}
  );

  return 0;
}