#pragma once

#include "engine/dom/node.h"

#include <string_view>

namespace speed::engine::html
{

class HtmlParser final
{
public:
  [[nodiscard]] dom::Node ParseFragment(std::string_view input) const;
};

} // namespace speed::engine::html
