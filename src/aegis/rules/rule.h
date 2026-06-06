#pragma once

#include <string>

namespace speed::aegis
{

struct Rule final
{
  std::string pattern;
  std::string reason;
};

} // namespace speed::aegis
