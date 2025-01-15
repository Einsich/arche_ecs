#include "ecs/logger.h"

struct Logger final : ecs::ILogger
{
  void log(const char *msg, int msg_len, ecs::LogType log_type) override
  {
    switch (log_type)
    {
    case ecs::LogType::Info:
      printf("\033[37mInfo: %.*s\033[39m\n", msg_len, msg);
      break;
    case ecs::LogType::Warning:
      printf("\033[33mWarning: %.*s\033[39m\n", msg_len, msg);
      break;
    case ecs::LogType::Error:
      printf("\033[31mError: %.*s\033[39m\n", msg_len, msg);
      break;
    }
  }
};