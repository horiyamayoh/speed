#pragma once

#include "base/ids/id_types.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace speed::browser
{

struct NavigationRequest final
{
  base::RequestId request_id;
  base::TabId tab_id;
  std::string url;
};

class NavigationController final
{
public:
  [[nodiscard]] std::string NormalizeForNavigation(std::string_view input) const;
  [[nodiscard]] std::optional<NavigationRequest> CreateNavigationRequest(base::TabId tab_id,
                                                                         std::string_view input);

private:
  std::uint64_t next_request_id_{1};
};

} // namespace speed::browser
