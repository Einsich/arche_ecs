#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <regex>
#include <filesystem>
#include <vector>
#include <stdarg.h>
#include "timer.h"

typedef unsigned int uint;
enum class ArgType
{
  ReadWrite, // T &t, T *t
  ReadOnly,  // const T &t, const T *t
  Copy       // T t, const T t
};
struct ParserFunctionArgument
{
  std::string type, name;
  bool reference = false;
  bool optional = false;
  bool is_const = false;
  ArgType argType = ArgType::ReadWrite;
  const char *get_type() const
  {
    if (optional)
    {
      return argType == ArgType::ReadWrite ?
            "ecs::Query::ComponentAccess::READ_WRITE_OPTIONAL" :
            "ecs::Query::ComponentAccess::READ_ONLY_OPTIONAL";
    }
    switch (argType)
    {
    case ArgType::ReadWrite:
      return "ecs::Query::ComponentAccess::READ_WRITE";
      break;
    case ArgType::ReadOnly:
      return "ecs::Query::ComponentAccess::READ_ONLY";
      break;
    case ArgType::Copy:
      return "ecs::Query::ComponentAccess::READ_COPY";
      break;
    }
  }
};

struct FileInfo
{
  std::string filePath;
  std::vector<int> newLinePositions;

  void scanFile(const std::string &file)
  {
    for (uint i = 0, n = file.size(); i < n; i++)
    {
      if (file[i] == '\n')
        newLinePositions.push_back(i);
    }
  }
  int findLine(int char_pos) const
  {
    auto it = std::lower_bound(newLinePositions.begin(), newLinePositions.end(), char_pos);
    return it - newLinePositions.begin() + 1;
  }
};

struct ParserSystemDescription
{
  std::string sys_file, sys_name, unique_name;
  std::vector<ParserFunctionArgument> args, req_args, req_not_args;
  std::vector<std::string> before, after, tags, on_event;
  std::string stage;
  std::string isJob;
};
#define SPACE_SYM " \n\t\r\a\f\v"
#define NAME_SYM "a-zA-Z0-9_"
#define SPACE "[" SPACE_SYM "]*"
#define NAME "[" NAME_SYM "]+"
#define ARGS "[" NAME_SYM "&*,:<>\\+\\-" SPACE_SYM "]*"

#define SYSTEM_LEXEMA NAME_SYM "&*,:<>\\]\\[" SPACE_SYM
#define SYSTEM_ANNOTATION "[" SYSTEM_LEXEMA "=;]*"
#define LEXEMA_ANNOTATION "[" SYSTEM_LEXEMA "=]+"
#define NEW_SYSTEM_ARGS "[(][" SYSTEM_LEXEMA "=;]*[)]"
#define VAR_NAME "[a-zA-Z]+[a-zA-Z0-9_]*"
#define VAR_TYPE "[a-zA-Z]+[a-zA-Z0-9_:]*"
#define TYPE_NAME VAR_TYPE SPACE VAR_NAME
#define TYPE_REGEX1 "(" VAR_TYPE SPACE "<" SPACE VAR_TYPE "(" SPACE "," SPACE VAR_TYPE ")*" SPACE ">|" VAR_TYPE ")"
#define TYPE_REGEX2 "(" VAR_TYPE SPACE "<" SPACE TYPE_REGEX1 "(" SPACE "," SPACE TYPE_REGEX1 ")*" SPACE ">|" VAR_TYPE ")" \
                    "|" VAR_NAME
#define SYSTEM_ARG "((" VAR_TYPE SPACE "<" SPACE TYPE_REGEX1 "(" SPACE "," SPACE TYPE_REGEX1 ")*" SPACE ">|" VAR_TYPE ")" SPACE VAR_NAME ")|" VAR_NAME

#define ARGS_L "[(]" ARGS "[)]"
#define ARGS_R "[\\[]" ARGS "[\\]]"
#define SYSTEM "ECS_SYSTEM" SPACE "[(]" SYSTEM_ANNOTATION "[)]"
#define QUERY "ECS_QUERY" SPACE "[(]" SYSTEM_ANNOTATION "[)]"
#define EVENT "ECS_EVENT" SPACE "[(]" SYSTEM_ANNOTATION "[)]"
#define REQUEST "ECS_REQUEST" SPACE "[(]" SYSTEM_ANNOTATION "[)]"

static const std::regex name_regex(NAME);
static const std::regex system_full_regex(SYSTEM SPACE NAME SPACE ARGS_L);
static const std::regex system_regex(SYSTEM);
static const std::regex query_full_regex(QUERY SPACE NAME SPACE "[(]" SPACE NAME SPACE "[,]" SPACE ARGS_R SPACE ARGS_L);
static const std::regex singl_query_full_regex(QUERY SPACE NAME SPACE "[(]" SPACE NAME SPACE "[,]" SPACE NAME SPACE "[,]" SPACE ARGS_R SPACE ARGS_L);
static const std::regex query_regex(QUERY);
static const std::regex event_full_regex(EVENT SPACE NAME SPACE ARGS_L);
static const std::regex event_regex(EVENT);
static const std::regex request_full_regex(REQUEST SPACE NAME SPACE ARGS_L);
static const std::regex request_regex(REQUEST);
static const std::regex args_regex(ARGS_L);
static const std::regex arg_regex("[" NAME_SYM "&*:<>" SPACE_SYM "]+");

static const std::regex new_system_args_regex(NEW_SYSTEM_ARGS);
static const std::regex new_system_annotation_regex(LEXEMA_ANNOTATION);
static const std::regex new_system_arg_regex(SYSTEM_ARG);
static const std::regex type_regex(TYPE_REGEX2);

enum CallableType
{
  SYSTEM_TYPE,
  EVENT_TYPE,
  QUERY_TYPE
};
namespace fs = std::filesystem;

constexpr int bufferSize = 1 << 10;
static char buffer[bufferSize];
static int error_count = 0, files_with_errors = 0;

static void log_success(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, bufferSize, fmt, args);
  va_end(args);
  printf("[Codegen] \033[32m%s\033[39m\n", buffer);
}

static void log_error(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, bufferSize, fmt, args);
  va_end(args);
  printf("[Codegen] \033[31m%s\033[39m\n", buffer);
  error_count++;
}

struct Match
{
  std::string::const_iterator begin, end;
  Match(std::string::const_iterator begin, std::string::const_iterator end)
      : begin(begin), end(end)
  {
  }
  Match(const std::string &str)
      : begin(str.begin()), end(str.end())
  {
  }
  std::string get() const
  {
    return std::string(begin, end);
  }
  bool empty() const
  {
    return begin == end;
  }
};

std::vector<Match> get_matches(Match str, const std::regex &reg, int max_matches = 10000000)
{
  std::vector<Match> v;
  std::sregex_iterator curMatch(str.begin, str.end, reg);
  std::sregex_iterator lastMatch;

  for (int i = 0; curMatch != lastMatch && i < max_matches; i++)
  {
    v.emplace_back(Match{str.begin + curMatch->position(), str.begin + curMatch->position() + curMatch->length()});
    curMatch++;
  }
  return v;
}

Match get_match(const Match &str, const std::regex &reg)
{
  std::vector<std::string> v;
  std::sregex_iterator curMatch(str.begin, str.end, reg);
  std::sregex_iterator lastMatch;

  Match range = str;
  if (curMatch != lastMatch)
  {
    range.begin += curMatch->position(0);
    range.end = range.begin + curMatch->length(0);
  }
  else
  {
    range.end = range.begin;
  }

  return range;
}
bool erase_substr(std::string &str, const std::string &to_erase)
{
  // Search for the substring in string
  size_t pos = str.find(to_erase);
  if (pos != std::string::npos)
  {
    // If found then erase it from string
    str.erase(pos, to_erase.length());
    return true;
  }
  return false;
}
ParserFunctionArgument clear_arg(std::string str)
{
  ParserFunctionArgument arg;

  bool optional = erase_substr(str, "*");
  bool ref = erase_substr(str, "&");
  bool const_ = erase_substr(str, "const");

  if (!optional && !ref)
  {
    arg.argType = ArgType::Copy;
  }
  else if ((ref || optional) && !const_)
  {
    arg.argType = ArgType::ReadWrite;
  }
  else
  {
    arg.argType = ArgType::ReadOnly;
  }
  arg.optional = optional;
  arg.reference = ref;
  arg.is_const = const_;

  auto args = get_matches(str, type_regex, 3);

  if (args.size() != 1 && args.size() != 2)
    return arg;
  arg.type = args[0].get();
  if (args.size() == 2)
    arg.name = args[1].get();
  return arg;
}
void parse_definition(Match &str, ParserSystemDescription &parserDescr)
{
  auto args_range = get_match(str, new_system_args_regex);

  // printf("%s\n", str.c_str());
  if (!args_range.empty())
  {
    auto args = get_matches(args_range, new_system_annotation_regex);
    auto system = parserDescr.sys_file.c_str();
    for (auto &arg : args)
    {
      auto args0 = get_matches(arg, new_system_arg_regex);

      if (args0.empty())
        log_error("expression parse failed \"%s\" in %s\n", arg.get().c_str(), system);
      else if (args0.size() == 1)
        log_error("argument \"%s\" without value in %s ", args0[0].get().c_str(), system);
      else
      {
        std::string key = args0[0].get();
        if (key == "tags")
        {
          for (uint i = 1; i < args0.size(); i++)
            parserDescr.tags.emplace_back(args0[i].get());
        }
        else if (key == "before")
        {
          for (uint i = 1; i < args0.size(); i++)
            parserDescr.before.emplace_back(args0[i].get());
        }
        else if (key == "after")
        {
          for (uint i = 1; i < args0.size(); i++)
            parserDescr.after.emplace_back(args0[i].get());
        }
        else if (key == "require")
        {
          for (uint i = 1; i < args0.size(); i++)
            parserDescr.req_args.emplace_back(clear_arg(args0[i].get()));
        }
        else if (key == "require_not")
        {
          for (uint i = 1; i < args0.size(); i++)
            parserDescr.req_not_args.emplace_back(clear_arg(args0[i].get()));
        }
        else if (key == "on_event")
        {
          for (uint i = 1; i < args0.size(); i++)
            parserDescr.on_event.emplace_back(args0[i].get());
        }
        else if (key == "job")
        {
          parserDescr.isJob = args0[1].get();
        }
        else if (key == "stage")
        {
          parserDescr.stage = args0[1].get();
          if (args0.size() != 2)
            log_error("wrong stages count in %s", system);

        }
        else
        {
          log_error("unsuported argument \"%s\" in %s", arg.get().c_str(), system);
        }
      }
    }
  }

  if (parserDescr.isJob.empty())
    parserDescr.isJob = "false";
}

void parse_system(std::vector<ParserSystemDescription> &systemsDescriptions,
                  const std::string &file,
                  const FileInfo &file_info,
                  const std::regex &full_regex, const std::regex &def_regex)
{

  auto systems = get_matches(file, full_regex);
  for (auto &system : systems)
  {
    auto definition_range = get_match({system.begin, system.end}, def_regex);
    auto name_range = get_match({definition_range.end, system.end}, name_regex);
    auto args_range = get_match({name_range.end, system.end}, args_regex);

    std::string systemPath = file_info.filePath + ":" + std::to_string(file_info.findLine(system.begin - file.begin()));

    std::replace(systemPath.begin(), systemPath.end(), '\\', '/');

    if (definition_range.empty())
    {
      log_error("system has wrong definition %s", systemPath.c_str());
      return;
    }
    if (name_range.empty())
    {
      log_error("system hasn't name %s", systemPath.c_str());
      return;
    }
    if (args_range.empty())
    {
      log_error("system hasn't correct argument list %s", systemPath.c_str());
      return;
    }

    args_range.begin++;
    args_range.end--;

    ParserSystemDescription descr;

    descr.sys_file = std::move(systemPath);
    descr.sys_name = name_range.get();
    descr.unique_name = descr.sys_file + "[" + descr.sys_name + "]";
    parse_definition(definition_range, descr);
    auto matched_args = get_matches(args_range, arg_regex);
    for (auto &arg : matched_args)
    {
      descr.args.push_back(clear_arg(arg.get()));
    }
    systemsDescriptions.emplace_back(std::move(descr));
  }
}

void template_query_types(std::ofstream &outFile, const ParserFunctionArgument *args, size_t args_count)
{
  for (uint i = 0; i < args_count; i++)
  {
    auto &arg = args[i];
    if (!arg.optional)
    {
      snprintf(buffer, bufferSize, "%s%s%s%s",
              arg.is_const ? "const " : "",
              arg.type.c_str(), "*",
              i + 1 == args_count ? "" : ", ");
    }
    else
    {
      snprintf(buffer, bufferSize, "ecs_details::PrtWrapper<%s%s>%s",
              arg.is_const ? "const " : "",
              arg.type.c_str(),
              i + 1 == args_count ? "" : ", ");
    }
    outFile << buffer;
  }
}

void write(std::ofstream &outFile, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, bufferSize, fmt, args);
  va_end(args);
  outFile << buffer;
}


static void query_forward_declaration(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    write(outFile,
          "template<typename Callable>\n"
          "static void %s(ecs::EcsManager &mgr, Callable &&query_function);\n\n",
          name);
  }
}


static void single_query_forward_declaration(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    write(outFile,
          "template<typename Callable>\n"
          "static void %s(ecs::EcsManager &mgr, ecs::EntityId eid, Callable &&query_function);\n\n",
          name);
  }
}

static void query_definition(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    write(outFile,
          "template<typename Callable>\n"
          "static void %s(ecs::EcsManager &mgr, Callable &&query_function)\n"
          "{\n"
          "  constexpr ecs::NameHash queryHash = ecs::hash(\"%s\");\n"
          "  const int N = %d;\n"
          "  ecs_details::query_iteration<N, ",
          name, query.unique_name.c_str(), query.args.size());
    template_query_types(outFile, query.args.data(), query.args.size());
    write(outFile,
          ">(mgr, queryHash, std::move(query_function));\n"
          "}\n\n");
  }
}

static void single_query_definition(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    write(outFile,
          "template<typename Callable>\n"
          "static void %s(ecs::EcsManager &mgr, ecs::EntityId eid, Callable &&query_function)\n"
          "{\n"
          "  constexpr ecs::NameHash queryHash = ecs::hash(\"%s\");\n"
          "  const int N = %d;\n"
          "  ecs_details::query_invoke_for_entity<N, ",
          name, query.unique_name.c_str(), query.args.size());
    template_query_types(outFile, query.args.data(), query.args.size());
    write(outFile,
          ">(mgr, eid, queryHash, std::move(query_function));\n"
          "}\n\n");
  }
}

static void system_definition(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    write(outFile,
          "static void %s_implementation(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component)\n"
          "{\n"
          "  const int N = %d;\n"
          "  ecs_details::query_archetype_iteration<N, ",
          name, query.args.size());
    template_query_types(outFile, query.args.data(), query.args.size());
    write(outFile,
          ">(archetype, to_archetype_component, %s, std::make_index_sequence<N>());\n"
          "}\n\n", name);
  }
}

static void event_definition(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    const char *event_type = query.args[0].type.c_str();
    const bool isAbstractEvent = query.args[0].type == "ecs::Event";
    write(outFile,
          "static void %s_broadcast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, ecs::EventId event_id, const void *event_ptr)\n"
          "{\n"
          "  ECS_UNUSED(event_id);\n"
          "  const int N = %d;\n"
          "  ecs_details::event_archetype_iteration<N, ",
          name, query.args.size() - 1);
    template_query_types(outFile, query.args.data() + 1, query.args.size() - 1);

    if (isAbstractEvent)
      write(outFile,
          ">(archetype, to_archetype_component, ecs::Event(event_id, event_ptr), %s, std::make_index_sequence<N>());\n"
          "}\n\n", name);
    else
      write(outFile,
          ">(archetype, to_archetype_component, *(const %s *)event_ptr, %s, std::make_index_sequence<N>());\n"
          "}\n\n", event_type, name);

    write(outFile,
          "static void %s_unicast_event(ecs_details::Archetype &archetype, const ecs::ToComponentMap &to_archetype_component, uint32_t component_idx, ecs::EventId event_id, const void *event_ptr)\n"
          "{\n"
          "  ECS_UNUSED(event_id);\n"
          "  const int N = %d;\n"
          "  ecs_details::event_invoke_for_entity<N, ",
          name, query.args.size() - 1);
    template_query_types(outFile, query.args.data() + 1, query.args.size() - 1);

    if (isAbstractEvent)
      write(outFile,
          ">(archetype, to_archetype_component, component_idx, ecs::Event(event_id, event_ptr), %s, std::make_index_sequence<N>());\n"
          "}\n\n", name);
    else
      write(outFile,
          ">(archetype, to_archetype_component, component_idx, *(const %s *)event_ptr, %s, std::make_index_sequence<N>());\n"
          "}\n\n", event_type, name);
  }
}

static void request_definition(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    const char *event_type = query.args[0].type.c_str();
    write(outFile,
          "static void %s_handler(ecs::Request &request)\n"
          "{\n"
          "  ecs_details::perform_request(reinterpret_cast<%s &>(request), %s__cache__, %s);\n"
          "}\n\n",
          name, event_type, name, name);
    write(outFile,
          "static void %s_single_handler(ecs::EntityId eid, ecs::Request &request)\n"
          "{\n"
          "  ecs_details::perform_request(eid, reinterpret_cast<%s &>(request), %s__cache__, %s);\n"
          "}\n\n",
          name, event_type, name, name);
  }
}

void fill_arguments(std::ofstream &outFile, const std::vector<ParserFunctionArgument> &args, bool event)
{
  uint i0 = event ? 1 : 0;

  write(outFile, "    {\n");
  for (uint i = i0; i < args.size(); i++)
  {
    auto &arg = args[i];

    write(outFile,
          "      {ecs::get_or_add_component(mgr, ecs::TypeInfo<%s>::typeId, \"%s\"), %s}%s\n",
          arg.type.c_str(),
          arg.name.c_str(),
          arg.get_type(),
          i + 1 == (uint)args.size() ? "" : ",");
  }
  write(outFile, "    };\n");
}

void fill_required_arguments(std::ofstream &outFile, const std::vector<ParserFunctionArgument> &args)
{
  write(outFile, "    {\n");
  for (uint i = 0; i < args.size(); i++)
  {
    auto &arg = args[i];

    write(outFile,
          "      ecs::get_or_add_component(mgr, ecs::TypeInfo<%s>::typeId, \"%s\")%s\n",
          arg.type.c_str(),
          arg.name.c_str(),
          i + 1 == (uint)args.size() ? "" : ",");
  }
  write(outFile, "    };\n");
}

static void fill_string_array(std::ofstream &outFile, const std::vector<std::string> &args)
{
  if (args.empty())
  {
    write(outFile, "  {},\n");
    return;
  }
  write(outFile, "  {");
  for (uint i = 0; i < args.size(); i++)
  {
    write(outFile,
          "\"%s\"%s",
          args[i].c_str(),
          i + 1 == (uint)args.size() ? "" : ", ");
  }
  write(outFile, "},\n");
}

static void fill_common_query_part(
    std::ofstream &outFile,
    const ParserSystemDescription &descr,
    const char *query_type,
    bool is_event)
{
  write(outFile,
        "  {\n"
        "    %s query;\n"
        "    query.uniqueName = \"%s\";\n"
        "    query.nameHash = ecs::hash(query.uniqueName.c_str());\n"
        "    query.querySignature =\n",
        query_type, descr.unique_name.c_str());
  fill_arguments(outFile, descr.args, is_event);
  write(outFile,
        "    query.requireComponents =\n");
  fill_required_arguments(outFile, descr.req_args);
  write(outFile,
        "    query.excludeComponents =\n");
  fill_required_arguments(outFile, descr.req_not_args);
}

static void fill_query_end(std::ofstream &outFile, const char *register_func)
{
  write(outFile,
        "    %s(mgr, std::move(query));\n"
        "  }\n",
        register_func);
}

void register_queries(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    fill_common_query_part(outFile, query, "ecs::Query", false);
    fill_query_end(outFile, "ecs::register_query");
  }
}

void register_systems(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    fill_common_query_part(outFile, query, "ecs::System", false);

    write(outFile,
          "    query.update_archetype = %s_implementation;\n",
          name);
    // write(outFile, "  \"%s\",\n", query.stage.c_str());
    // fill_string_array(outFile, query.before);
    // fill_string_array(outFile, query.after);
    // fill_string_array(outFile, query.tags);
    // write(outFile, "  &%s_implementation);\n\n", name);
    fill_query_end(outFile, "ecs::register_system");
  }
}

void register_events(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    const char *event_type = query.args[0].type.c_str();
    const bool isAbstractEvent = query.args[0].type == "ecs::Event";

    fill_common_query_part(outFile, query, "ecs::EventHandler", true);
    write(outFile,
          "    query.broadcastEvent = %s_broadcast_event;\n"
          "    query.unicastEvent = %s_unicast_event;\n",
          name, name, event_type);
    if (isAbstractEvent)
    {
        write(outFile,
          "    query.eventIds = {");
      for (uint i = 0; i < query.on_event.size(); i++)
      {
        write(outFile,
              "ecs::EventInfo<%s>::eventId, ",
              query.on_event[i].c_str());
      }
      write(outFile, "};\n");
    }
    else
    {
      write(outFile,
          "    query.eventIds = {ecs::EventInfo<%s>::eventId};\n",
          event_type);
    }

    // fill_string_array(outFile, query.before);
    // fill_string_array(outFile, query.after);
    // fill_string_array(outFile, query.tags);
    // write(outFile,
    //       "  &%s_handler, &%s_single_handler,\n"
    //       "  ecs::EventIndex<%s>::value);\n\n",
    //       name, name, event_type);
    fill_query_end(outFile, "ecs::register_event");
  }
}

void register_requests(std::ofstream &outFile, const std::vector<ParserSystemDescription> &descr)
{
  for (auto &query : descr)
  {
    const char *name = query.sys_name.c_str();
    const char *event_type = query.args[0].type.c_str();
    fill_common_query_part(outFile, query, "ecs::Request", true);
    fill_string_array(outFile, query.before);
    fill_string_array(outFile, query.after);
    fill_string_array(outFile, query.tags);
    fill_query_end(outFile, "ecs::register_request");
    write(outFile,
          "  &%s_handler, &%s_single_handler,\n"
          "  ecs::RequestIndex<%s>::value);\n\n",
          name, name, event_type);
  }
}

static int processed_files = 0;

void process_inl_file(const fs::path &path, const fs::path &root)
{
  Timer t;
  std::ifstream inFile;
  inFile.open(path); // open the input file

  std::stringstream strStream;
  strStream << inFile.rdbuf();       // read the file
  std::string str = strStream.str(); // str holds the content of the file

  std::vector<ParserSystemDescription> systemsDescriptions;
  std::vector<ParserSystemDescription> queriesDescriptions;
  std::vector<ParserSystemDescription> singleQueriesDescriptions;
  std::vector<ParserSystemDescription> eventsDescriptions;
  std::vector<ParserSystemDescription> requestDescriptions;
  std::string pathStr = path.string();
  FileInfo fileInfo;
  fileInfo.filePath = fs::relative(path, root).string();
  fileInfo.scanFile(str);

  parse_system(systemsDescriptions, str, fileInfo, system_full_regex, system_regex);
  parse_system(queriesDescriptions, str, fileInfo, query_full_regex, query_regex);
  parse_system(singleQueriesDescriptions, str, fileInfo, singl_query_full_regex, query_regex);
  parse_system(eventsDescriptions, str, fileInfo, event_full_regex, event_regex);
  parse_system(requestDescriptions, str, fileInfo, request_full_regex, request_regex);

  std::ofstream outFile;
  outFile.open(pathStr + ".cpp", std::ios::out);
  outFile << "#include <ecs/codegen_helpers.h>\n";
  query_forward_declaration(outFile, queriesDescriptions);
  single_query_forward_declaration(outFile, singleQueriesDescriptions);
  outFile << "#include " << path.filename() << "\n";
  outFile << "//Code-generator production\n\n";


  query_definition(outFile, queriesDescriptions);
  single_query_definition(outFile, singleQueriesDescriptions);
  system_definition(outFile, systemsDescriptions);
  event_definition(outFile, eventsDescriptions);
  request_definition(outFile, requestDescriptions);

  std::string fileName = path.stem().string();
  outFile << "static void ecs_registration(ecs::EcsManager &mgr)\n"
             "{\n"
             "  ECS_UNUSED(mgr);\n";

  register_queries(outFile, queriesDescriptions);
  register_queries(outFile, singleQueriesDescriptions);
  register_systems(outFile, systemsDescriptions);
  register_events(outFile, eventsDescriptions);
  register_requests(outFile, requestDescriptions);

  outFile << "}\n";
  outFile << "static ecs_details::CodegenFileRegistration fileRegistration(&ecs_registration);\n";
  outFile << "ECS_PULL_DEFINITION(" << ("variable_pull_" + fileName) << ")\n";
  outFile.close();

  if (error_count == 0)
    log_success("processed %s in %d ms", pathStr.c_str(), t.get_time());
  else
    log_error("processed %s in %d ms", pathStr.c_str(), t.get_time());
  files_with_errors += error_count > 0;
  processed_files += 1;
  error_count = 0;
}

void process_folder(const std::string &path, const fs::path &root, bool force_rebuild)
{
  if (!fs::exists(path))
  {
    printf("Didn't exist path %s\n", path.c_str());
    return;
  }
  for (auto &p : fs::recursive_directory_iterator(path))
  {

    if (p.is_regular_file() && p.path().has_extension())
    {
      if (p.path().extension() == ".inl")
      {
        fs::path cpp_file = fs::path(p.path().string() + ".cpp");
        bool requireCodeGen =
            force_rebuild ||
            !fs::exists(cpp_file) ||
            fs::last_write_time(cpp_file) < p.last_write_time();

        if (requireCodeGen)
          process_inl_file(p.path(), root);
      }
    }
  }
}
int main(int argc, char **argv)
{
  Timer t;
  std::vector<std::string> paths;

  std::string_view forceRebuildStr = "-force_rebuild";
  std::string_view rootArg = "-root=";
  fs::path root = fs::current_path();
  bool forceRebuild = false;
  for (int i = 1; i < argc; i++)
  {
    if (argv[i] == forceRebuildStr)
      forceRebuild = true;
    else if (strstr(argv[i], rootArg.data()) == argv[i])
    {
      root = argv[i] + rootArg.size();
    }
    else
      paths.emplace_back(argv[i]);
  }

  printf("root %ls\n", root.c_str());

  for (const std::string &path : paths)
    process_folder(path, root, forceRebuild);

  if (processed_files == 0)
  {
    printf("[Codegen] no work\n");
    return 0;
  }

  if (files_with_errors == 0)
    log_success("successfully processed %d files in %d ms", processed_files, t.get_time());
  else
    log_error("processed %d files with %d errors in %d ms", processed_files, files_with_errors, t.get_time());

  std::cout << std::endl;
}
