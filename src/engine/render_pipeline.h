#pragma once

#include "engine/dom/node.h"
#include "engine/layout/layout_engine.h"
#include "engine/paint/display_list.h"

#include <string_view>

namespace speed::engine
{

struct RenderResult final
{
  layout::LayoutResult layout;
  paint::DisplayList display_list;
};

class RenderPipeline final
{
public:
  [[nodiscard]] RenderResult RenderHtml(std::string_view document_body,
                                        layout::Viewport viewport) const;
  [[nodiscard]] RenderResult RenderDocument(const dom::Node& document,
                                            layout::Viewport viewport) const;
};

} // namespace speed::engine
