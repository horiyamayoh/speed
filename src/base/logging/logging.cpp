#include "base/logging/logging.h"

#include <iostream>

namespace speed::base
{

const char* ToString(LogLevel level)
{
  switch (level)
  {
  case LogLevel::kInfo:
    return "info";
  case LogLevel::kWarning:
    return "warning";
  case LogLevel::kError:
    return "error";
  }

  return "unknown";
}

void Log(LogLevel level, std::string_view component, std::string_view message)
{
  std::clog << '[' << ToString(level) << "] " << component << ": " << message << '\n';
}

} // namespace speed::base
