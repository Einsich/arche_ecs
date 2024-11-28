#include "ecs/ecs.h"
#include "ecs/config.h"
#include "ecs/type_declaration.h"
#include "ecs/component_declaration.h"
#include "ecs/component_init.h"
#include "ecs/entity_id.h"
#include <assert.h>

#include <numeric>
#include <new>
#include <span>
#include <array>
namespace ecs
{

using Type = ska::flat_hash_map<ComponentId, TypeId  /*ComponentId already has TypeId*/>;

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
  ComponentId componentId;

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

  uint32_t entityCount = 0;
  uint32_t chunkSize = 0;
  uint32_t capacity = 0;
  uint32_t chunkCount = 0;

  Archetype() = default;
  Archetype(const TypeDeclarationMap &type_map, Type &&_type, ArchetypeChunkSize chunk_size_power) : type(std::move(_type))
  {
    assert(!type.empty());
    archetypeId = get_archetype_id(type);
    collumns.reserve(type.size());
    componentToCollumnIndex.reserve(type.size());
    for (auto [componentId, typeId] : type)
    {
      const TypeDeclaration *typeDeclaration = type_map.find(typeId)->second.get();
      collumns.emplace_back(chunk_size_power, typeDeclaration->sizeOfElement, typeDeclaration->alignmentOfElement, typeId);
      Collumn &collumn = collumns.back();
      collumn.debugName = typeDeclaration->typeName;
      collumn.componentId = componentId;
      componentToCollumnIndex[componentId] = collumns.size() - 1;
    }
    chunkSize = collumns[0].chunkSize;
  }

  int get_component_index(ComponentId componentId) const
  {
    auto it = componentToCollumnIndex.find(componentId);
    return it != componentToCollumnIndex.end() ? it->second : -1;
  }

  void try_add_chunk(int requiredEntityCount)
  {
    while (entityCount + requiredEntityCount > capacity)
    {
      capacity += chunkSize;
      chunkCount++;
      for (Collumn &collumn : collumns)
      {
        collumn.add_chunk();
        assert(capacity == collumn.capacity);
      }
    }
  }

  // return index of the added entity
  void add_entity(const TypeDeclarationMap &type_map, const InitializerList &template_init, InitializerList override_list)
  {
    try_add_chunk(1);
    for (Collumn &collumn : collumns)
    {

      const TypeDeclaration *typeDeclaration = type_map.find(collumn.typeId)->second.get();
      // firstly check initialization data in override_list and move it
      auto it = override_list.args.find(collumn.componentId);
      if (it != override_list.args.end())
      {
        typeDeclaration->move_construct(collumn.get_data(entityCount), it->second.data);
        continue;
      }
      // then check initialization data in template_init and copy it
      auto it2 = template_init.args.find(collumn.componentId);
      if (it2 != template_init.args.end())
      {
        typeDeclaration->copy_construct(collumn.get_data(entityCount), it2->second.data);
        continue;
      }
      // if there is no initialization data, construct default
      typeDeclaration->construct_default(collumn.get_data(entityCount));
      // but this is error because we have to have initialization data for all components
      printf("[ECS] Error: no initialization data for component %s\n", collumn.debugName.c_str());
    }
    entityCount++;
  }

  void add_entities(const TypeDeclarationMap &type_map, const InitializerList &template_init, InitializerSoaList override_soa_list)
  {
    int requiredEntityCount = override_soa_list.size();
    try_add_chunk(requiredEntityCount);
    for (Collumn &collumn : collumns)
    {
      const TypeDeclaration *typeDeclaration = type_map.find(collumn.typeId)->second.get();

      // firstly check initialization data in override_soa_list and move it
      auto it = override_soa_list.args.find(collumn.componentId);
      if (it != override_soa_list.args.end())
      {
        ComponentDataSoa &componentDataSoa = it->second;
        for (int i = 0; i < requiredEntityCount; i++)
        {
          typeDeclaration->move_construct(collumn.get_data(entityCount + i), componentDataSoa.data[i]);
        }
        continue;
      }
      // then check initialization data in template_init and copy it
      auto it2 = template_init.args.find(collumn.componentId);
      if (it2 != template_init.args.end())
      {
        const ComponentData &componentData = it2->second;
        for (int i = 0; i < requiredEntityCount; i++)
        {
          typeDeclaration->copy_construct(collumn.get_data(entityCount + i), componentData.data);
        }
        continue;
      }
      // if there is no initialization data, construct default
      for (int i = 0; i < requiredEntityCount; i++)
      {
        typeDeclaration->construct_default(collumn.get_data(entityCount + i));
      }
      // but this is error because we have to have initialization data for all components
      printf("[ECS] Error: no initialization data for component %s\n", collumn.debugName.c_str());
    }
    entityCount += requiredEntityCount;
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

struct Template
{
  std::string name;
  InitializerList args;
  ArchetypeId archetypeId;
  std::vector<TemplateId> composition;
};

struct EcsManager
{
  TypeDeclarationMap typeMap;
  ComponentDeclarationMap componentMap;
  ska::flat_hash_map<ArchetypeId, Archetype> archetypeMap;
  ska::flat_hash_map<uint32_t, Query> queries;
  EntityContainer entityContainer;
  ska::flat_hash_map<TemplateId, Template> templates;

  ecs::TypeId EntityIdTypeId;
  ecs::ComponentId eidComponentId;

  EcsManager()
  {
    EntityIdTypeId = ecs::type_registration<ecs::EntityId>(typeMap, "EntityId");
    eidComponentId = ecs::component_registration(componentMap, EntityIdTypeId, "eid");
  }

  ecs::EntityId create_entity(ArchetypeId archetypeId, const InitializerList &template_init, InitializerList override_list)
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
    override_list.push_back(ecs::ComponentInit(eidComponentId, ecs::EntityId(eid)));
    assert(archetype.type.size() == template_init.size());
    archetype.add_entity(typeMap, template_init, std::move(override_list));
    return eid;
  }
  ecs::EntityId create_entity(TemplateId templateId, InitializerList init_list = {})
  {
    auto it = templates.find(templateId);
    if (it == templates.end())
    {
      printf("[ECS] Error: Template not found\n");
      return EntityId();
    }
    const Template &templateRecord = it->second;
    return create_entity(templateRecord.archetypeId, templateRecord.args, std::move(init_list));
  }

  std::vector<EntityId> create_entities(ArchetypeId archetypeId, const InitializerList &template_init, InitializerSoaList override_soa_list)
  {
    auto it = archetypeMap.find(archetypeId);
    if (it == archetypeMap.end())
    {
      printf("Archetype not found\n");
      return {};
    }
    Archetype &archetype = it->second;
    int requiredEntityCount = override_soa_list.size();
    std::vector<EntityId> eids(requiredEntityCount);
    uint32_t entityIndex = archetype.entityCount;
    for (int i = 0; i < requiredEntityCount; i++)
    {
      eids[i] = entityContainer.create_entity(archetypeId, entityIndex + i);
    }

    // need to create copy to return list of eids
    override_soa_list.push_back(ecs::ComponentSoaInit(eidComponentId, std::vector<EntityId>(eids)));
    assert(archetype.type.size() == template_init.size());
    archetype.add_entities(typeMap, template_init, std::move(override_soa_list));
    return eids;
  }

  std::vector<EntityId> create_entities(TemplateId templateId, InitializerSoaList init_soa_list)
  {
    auto it = templates.find(templateId);
    if (it == templates.end())
    {
      printf("[ECS] Error: Template not found\n");
      return {};
    }
    const Template &templateRecord = it->second;
    return create_entities(templateRecord.archetypeId, templateRecord.args, std::move(init_soa_list));
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


ArchetypeId get_or_create_archetype(EcsManager &manager, const InitializerList &components, ArchetypeChunkSize chunk_size_power)
{
  Type type;
  type.reserve(components.size() + 1);
  for (const auto &[componentId, component] : components.args)
  {
    type[componentId] = manager.componentMap.find(componentId)->second->typeId;
  }

  ArchetypeId archetypeId = get_archetype_id(type);

  if (manager.archetypeMap.find(archetypeId) == manager.archetypeMap.end())
  {
    manager.archetypeMap[archetypeId] = Archetype(manager.typeMap, std::move(type), chunk_size_power);
  }

  return archetypeId;
}

//1)  A=1
//2)  A=2 B=2
//3)  A=3 B=3 C=3
//4)  A=4 B=4 C=4 D=4

// "X" {1, 2, 3, 4}
// A=1 B=2 C=3 D=4
// "Y" {2, 1, 3, 4}
// A=2 B=2 C=3 D=4

TemplateId template_registration(EcsManager &manager, const char *_name, const std::span<TemplateId> &parent_templates, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands)
{
  TemplateId templateId = hash(_name);
  if (manager.templates.find(templateId) != manager.templates.end())
  {
    printf("Template \"%s\" already exists\n", _name);
    return templateId;
  }
  components.push_back(ecs::ComponentInit{manager.eidComponentId, ecs::EntityId()});
  for (TemplateId parent_template : parent_templates)
  {
    auto it = manager.templates.find(parent_template);
    if (it == manager.templates.end())
    {
      printf("Parent template not found\n");
      return templateId;
    }
    const Template &parentTemplate = it->second;
    for (const auto &[componentId, componentInit] : parentTemplate.args.args)
    {
      if (components.args.count(componentId) > 0)
      {
        continue;
      }
      else
      {
        components.push_back({componentId, componentInit.copy()});
      }
    }
  }
  ArchetypeId archetypeId = get_or_create_archetype(manager, components, chunk_size_power);

  Template templateRecord{std::string(_name), std::move(components), archetypeId, {}};

  manager.templates[templateId] = std::move(templateRecord);

  return templateId;
}

TemplateId template_registration(EcsManager &manager, const char *_name, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands)
{
  return template_registration(manager, _name, {}, std::move(components), chunk_size_power);
}

TemplateId template_registration(EcsManager &manager, const char *_name, TemplateId parent_template, InitializerList &&components, ArchetypeChunkSize chunk_size_power = ArchetypeChunkSize::Thousands)
{
  return template_registration(manager, _name, {&parent_template, 1}, std::move(components), chunk_size_power);
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


void print_name(const std::string &name, float3 position, int *health)
{
  printf("print_name [%s] (%f %f %f), %d\n", name.c_str(), position.x, position.y, position.z, health ? *health : -1);
}

// user_code.cpp.gen
// #include "user_code.cpp.inl"

// user_code.cpp.gen
// #include "user_code.cpp.inl"


template<typename T>
struct PrtWrapper
{
  T *ptr;
  PrtWrapper(T *ptr) : ptr(ptr) {}
  PrtWrapper(char *ptr) : ptr((T*)ptr) {}
  T *operator*() { return ptr; }
  void operator++() { ptr = ptr ? ptr + 1 : ptr; }
};


template<typename Callable, typename ...NoCRefPtrArgs>
void perform_system(Callable &&callable_query, size_t elementCount, NoCRefPtrArgs ...components)
{
  for (size_t i = 0; i < elementCount; i++)
  {
    callable_query(*(components)...);
    ((++components), ...);
  }
}
template<size_t N, typename ...CastArgs, typename Callable, std::size_t... I>
void templated_archetype_iterate(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, Callable &&callable_query, std::index_sequence<I...>)
{
  int componenIdx[N] = {to_archetype_component[I]...};

  char **chunks[N] = {
    (componenIdx[I] >= 0 ? archetype.collumns[componenIdx[I]].chunks.data() : nullptr)...
  };

  for (uint32_t chunkIdx = 0, chunkCount = archetype.chunkCount; chunkIdx < chunkCount; chunkIdx++)
  {
    uint32_t elementCount = std::min(archetype.entityCount - archetype.chunkSize * chunkIdx, archetype.chunkSize);
    perform_system(std::move(callable_query), elementCount, (CastArgs)(chunks[I] ? chunks[I][chunkIdx] : nullptr)...);
  }
}


void update_archetype_codegen(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  templated_archetype_iterate<N, ecs::EntityId *, float3 *, const float3 *>(archetype, to_archetype_component, update, std::make_index_sequence<N>());
}

void print_name_archetype_codegen(ecs::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)
{
  const int N = 3;
  templated_archetype_iterate<N, std::string *, float3 *, PrtWrapper<int>>(archetype, to_archetype_component, print_name, std::make_index_sequence<N>());
}


#include "../../timer.h"

template<typename Callable>
static void print_name(ecs::EcsManager &, Callable &&);

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

  ecs::TemplateId template1 = template_registration(mgr, "point",
    {{
      {positionId, float3{}},
      {velocityId, float3{}}
    }}, ecs::ArchetypeChunkSize::Medium);

  ecs::TemplateId template2 = template_registration(mgr, "named_point",
    {{
      {velocityId, float3{}},
      {positionId, float3{}},
      {nameId, std::string{}}
    }}, ecs::ArchetypeChunkSize::Medium);

  ecs::TemplateId template3 = template_registration(mgr, "named_alive_point",
    {{
      {velocityId, float3{}},
      {positionId, float3{}},
      {nameId, std::string{}},
      {healthId, 0}
    }}, ecs::ArchetypeChunkSize::Medium);

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
    // mgr.queries[query.nameHash] = std::move(query); // TODO: fix this
    mgr.queries[query.nameHash] = query;
    update_position_query = std::move(query);
  }
  {
    ecs::Query query;
    query.uniqueName = __FILE__ LINE_STRING "print_name";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature = {
      {nameId, ecs::Query::ComponentAccess::READ_ONLY},
      {positionId, ecs::Query::ComponentAccess::READ_COPY},
      {healthId, ecs::Query::ComponentAccess::READ_ONLY_OPTIONAL},
    };
    query.requireComponents = {};
    query.excludeComponents = {};
    query.update_archetype = print_name_archetype_codegen;
    for (auto &[id, archetype]  : mgr.archetypeMap)
    {
      query.try_registrate(archetype);
    }
    // mgr.queries[query.nameHash] = std::move(query); // TODO: fix this
    mgr.queries[query.nameHash] = query;
    print_name_query = std::move(query);
  }
  {
    ecs::Query query;
    query.uniqueName = "print_name_query";
    query.nameHash = ecs::hash(query.uniqueName.c_str());
    query.querySignature = {
      {nameId, ecs::Query::ComponentAccess::READ_ONLY},
      {healthId, ecs::Query::ComponentAccess::READ_ONLY_OPTIONAL}
    };
    query.requireComponents = {};
    query.excludeComponents = {};
    query.update_archetype = nullptr;

    for (auto &[id, archetype]  : mgr.archetypeMap)
    {
      query.try_registrate(archetype);
    }
    mgr.queries[query.nameHash] = std::move(query);
  }

  //unfortunately, we can't use initializer_list here, because it doesn't support move semantics
  // _CONSTEXPR20 vector(initializer_list<_Ty> _Ilist, const _Alloc& _Al = _Alloc())

  ecs::InitializerList args;
  args.push_back(ecs::ComponentInit{positionId, float3{1, 2, 3}});
  args.push_back(ecs::ComponentInit{velocityId, float3{4, 5, 6}});
  mgr.create_entity(template1, std::move(args));


  mgr.create_entity(
    template3,
    ecs::InitializerList{
      {
        ecs::ComponentInit{positionId, float3{7, 8, 9}},
        ecs::ComponentInit{velocityId, float3{10, 11, 12}},
        ecs::ComponentInit{healthId, 100},
        ecs::ComponentInit{nameId, std::string("entity1")}
      }
    }
  );

  ecs::TemplateId movableTemplate = template_registration(mgr, "movable",
    {{
      {positionId, float3{0, 0, 0}},
      {velocityId, float3{0, 0, 0}}
    }}
  );
  ecs::TemplateId brickTemplate = template_registration(mgr, "brick", movableTemplate,
    {{
      {positionId, float3{1, 0, 0}},
      {healthId, 10},
      {nameId, std::string("brick")}
    }}
  );
  ecs::TemplateId humanTemplate = template_registration(mgr, "human", movableTemplate,
    {{
      {positionId, float3{2, 0, 0}},
      // {healthId, 50},
      {nameId, std::string("human")}
    }}
  );
  ecs::TemplateId bigBrickTemplate = template_registration(mgr, "bigbrick", brickTemplate,
    {{
      {positionId, float3{3, 0, 0}},
      {healthId, 100}
    }}
  );

  mgr.create_entity(movableTemplate);
  mgr.create_entity(brickTemplate);
  mgr.create_entity(humanTemplate);
  mgr.create_entity(bigBrickTemplate);


  // if (false)
  {
    Timer timer("create_entity"); // (0.066700 ms)
    for (int i = 0; i < 100; i++)
    {
      float f = i;
      ecs::EntityId eid = mgr.create_entity(
        template2,
        {{
          {positionId, float3{f, f, f}},
          // {velocityId, float3{f, f, f}},
          {nameId, std::string("very_long_node_name") + std::to_string(i)}
        }}
      );
      UNUSED(eid);
    }
  }

  // if (false)
  {
    Timer timer("create_entities"); // (0.046000 ms)
    std::vector<float3> positions;
    std::vector<std::string> names;
    positions.reserve(100);
    names.reserve(100);
    for (int i = 0; i < 100; i++)
    {
      float f = i;
      positions.push_back({f, f, f});
      names.push_back("soa_node" + std::to_string(i));
    }
    ecs::InitializerSoaList init;
    mgr.create_entities(
      template2,
      {{
          {positionId, std::move(positions)},
          {nameId, std::move(names)}
      }}
    );
  }

  // assert(mgr.destroy_entity(eid1));

  ecs::update_query(mgr.archetypeMap, update_position_query);
  ecs::update_query(mgr.archetypeMap, print_name_query);

  print_name(mgr, [](const std::string &name, int *health)
  {
    printf("print_name [%s] %d\n", name.c_str(), health ? *health : -1);
  });


  return 0;
}

//
template<typename Callable>
static void print_name(ecs::EcsManager &mgr, Callable &&query_function)
{
  constexpr ecs::NameHash queryHash = ecs::hash("print_name_query");
  auto it = mgr.queries.find(queryHash);
  if (it != mgr.queries.end())
  {
    ecs::Query &query = it->second;

    for (const auto &archetypeRecord : query.archetypesCache)
    {
      auto it = mgr.archetypeMap.find(archetypeRecord.archetypeId);
      if (it == mgr.archetypeMap.end())
      {
        continue;
      }

      ecs::Archetype &archetype = it->second;
      const ecs::ToComponentMap &to_archetype_component = archetypeRecord.toComponentIndex;
      const int N = 2;
      templated_archetype_iterate<N, std::string *, PrtWrapper<int>>(archetype, to_archetype_component, query_function, std::make_index_sequence<N>());
    }
  }
}