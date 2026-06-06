#pragma once

#include <cstdint>
#include <string_view>

namespace speed::base
{

enum class LogLevel : std::uint8_t
{
  kInfo,
  kWarning,
  kError,
};

[[nodiscard]] const char* ToString(LogLevel level);
void Log(LogLevel level, std::string_view component, std::string_view message);

} // namespace speed::base
