#pragma once


namespace ecs
{
  enum class LogType
  {
    Info,
    Warning,
    Error
  };
  enum class LogLevel
  {
    Quite,
    Verbose
  };

  struct ILogger
  {
    virtual void log(const char *msg, int msg_len, LogType log_type) = 0;
    virtual ~ILogger() = default;
  };

}

namespace ecs_details
{

  struct LoggerAdapter
  {
    ecs::LogType currentLogType = ecs::LogType::Info;
    ecs::ILogger *logger;

    void log(const char *fmt, ...);
  };

  #define ECS_LOG_INFO_VERBOSE(mgr) if (mgr.logger && mgr.currentLogLevel == ecs::LogLevel::Verbose) ecs_details::LoggerAdapter{ecs::LogType::Info, mgr.logger.get()}
  #define ECS_LOG_INFO(mgr) if (mgr.logger) ecs_details::LoggerAdapter{ecs::LogType::Info, mgr.logger.get()}
  #define ECS_LOG_WARNING(mgr) if (mgr.logger) ecs_details::LoggerAdapter{ecs::LogType::Warning, mgr.logger.get()}
  #define ECS_LOG_ERROR(mgr) if (mgr.logger) ecs_details::LoggerAdapter{ecs::LogType::Error, mgr.logger.get()}
}