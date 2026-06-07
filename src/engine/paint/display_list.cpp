#include "engine/paint/display_list.h"

#include <algorithm>

namespace
{

[[nodiscard]] bool HasBorder(const speed::engine::style::Edges& border_width)
{
  return border_width.top > 0 || border_width.right > 0 || border_width.bottom > 0 ||
         border_width.left > 0;
}

[[nodiscard]] bool HasPaintableRect(const speed::engine::layout::Rect& rect)
{
  return rect.width > 0 && rect.height > 0;
}

void PaintBox(const speed::engine::layout::LayoutBox& box,
              speed::engine::paint::DisplayList& display_list)
{
  if (!HasPaintableRect(box.rect))
  {
    return;
  }

  if (box.type == speed::engine::layout::LayoutBoxType::kText)
  {
    display_list.commands.push_back({
        .type = speed::engine::paint::DisplayCommandType::kText,
        .rect = box.rect,
        .color = box.style.color,
        .border_width = {},
        .font_size_px = box.style.font_size_px,
        .text = box.text,
    });
    return;
  }

  if (!speed::engine::style::IsTransparent(box.style.background_color))
  {
    display_list.commands.push_back({
        .type = speed::engine::paint::DisplayCommandType::kRect,
        .rect = box.rect,
        .color = box.style.background_color,
        .border_width = {},
        .font_size_px = box.style.font_size_px,
        .text = {},
    });
  }

  if (HasBorder(box.style.border_width))
  {
    display_list.commands.push_back({
        .type = speed::engine::paint::DisplayCommandType::kBorder,
        .rect = box.rect,
        .color = box.style.border_color,
        .border_width = box.style.border_width,
        .font_size_px = box.style.font_size_px,
        .text = {},
    });
  }

  if (box.type == speed::engine::layout::LayoutBoxType::kImagePlaceholder)
  {
    display_list.commands.push_back({
        .type = speed::engine::paint::DisplayCommandType::kImagePlaceholder,
        .rect = box.rect,
        .color = speed::engine::style::Color{.red = 160, .green = 160, .blue = 160, .alpha = 255},
        .border_width = {},
        .font_size_px = box.style.font_size_px,
        .text = {},
    });
  }

  for (const speed::engine::layout::LayoutBox& child : box.children)
  {
    PaintBox(child, display_list);
  }
}

} // namespace

namespace speed::engine::paint
{

DisplayList Painter::Paint(const layout::LayoutResult& layout) const
{
  DisplayList display_list;
  for (const layout::LayoutBox& child : layout.root.children)
  {
    PaintBox(child, display_list);
  }
  return display_list;
}

} // namespace speed::engine::paint
