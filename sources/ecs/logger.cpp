#include "ecs/logger.h"

#include <stdarg.h>
#include <cstdio>
#include <assert.h>

namespace ecs_details
{
  void LoggerAdapter::log(const char *fmt, ...)
  {
    const int bufferSize = 1024;
    char buffer[bufferSize];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, bufferSize, fmt, args);
    va_end(args);
    assert(logger);
    logger->log(buffer, len, currentLogType);
  }
}