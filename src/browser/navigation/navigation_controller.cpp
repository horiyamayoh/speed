#include "browser/navigation/navigation_controller.h"

#include <string>

namespace speed::browser
{

std::string NavigationController::NormalizeForNavigation(std::string_view input) const
{
  if (input.find("://") != std::string_view::npos)
  {
    return std::string(input);
  }

  return "https://" + std::string(input);
}

} // namespace speed::browser
