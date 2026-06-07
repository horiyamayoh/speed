#include "engine/render_pipeline.h"

#include "engine/html/html_parser.h"
#include "engine/style/style_resolver.h"

#include <utility>

namespace speed::engine
{

RenderResult RenderPipeline::RenderHtml(std::string_view document_body,
                                        layout::Viewport viewport) const
{
  const html::HtmlParser parser;
  const dom::Node document = parser.ParseFragment(document_body);
  return RenderDocument(document, viewport);
}

RenderResult RenderPipeline::RenderDocument(const dom::Node& document,
                                            layout::Viewport viewport) const
{
  const style::StyleResolver style_resolver;
  const style::StyledNode styled_document = style_resolver.Resolve(document);

  const layout::LayoutEngine layout_engine;
  layout::LayoutResult layout = layout_engine.Layout(styled_document, viewport);

  const paint::Painter painter;
  paint::DisplayList display_list = painter.Paint(layout);

  return {
      .layout = std::move(layout),
      .display_list = std::move(display_list),
  };
}

} // namespace speed::engine
