#pragma once

#include <string>
#include <string_view>

namespace speed::browser
{

class NavigationController final
{
public:
  [[nodiscard]] std::string NormalizeForNavigation(std::string_view input) const;
};

} // namespace speed::browser
