#include "ui/gui/display_list_renderer.h"

#include <algorithm>

namespace speed::ui::gui
{

namespace
{

[[nodiscard]] platform::window::Color ToPlatformColor(engine::style::Color color)
{
  return {
      .red = color.red,
      .green = color.green,
      .blue = color.blue,
      .alpha = color.alpha,
  };
}

[[nodiscard]] platform::window::Rect
TranslateRect(engine::layout::Rect rect, platform::window::Rect viewport, int scroll_y)
{
  return {
      .x = viewport.x + rect.x,
      .y = viewport.y + rect.y - scroll_y,
      .width = rect.width,
      .height = rect.height,
  };
}

[[nodiscard]] bool Intersects(platform::window::Rect left, platform::window::Rect right)
{
  return left.width > 0 && left.height > 0 && right.width > 0 && right.height > 0 &&
         left.x < right.x + right.width && left.x + left.width > right.x &&
         left.y < right.y + right.height && left.y + left.height > right.y;
}

void DrawBorder(platform::window::Canvas& canvas,
                platform::window::Rect rect,
                platform::window::Color color,
                engine::style::Edges border_width)
{
  if (border_width.top > 0)
  {
    canvas.FillRect({.x = rect.x, .y = rect.y, .width = rect.width, .height = border_width.top},
                    color);
  }

  if (border_width.right > 0)
  {
    canvas.FillRect(
        {
            .x = rect.x + rect.width - border_width.right,
            .y = rect.y,
            .width = border_width.right,
            .height = rect.height,
        },
        color);
  }

  if (border_width.bottom > 0)
  {
    canvas.FillRect(
        {
            .x = rect.x,
            .y = rect.y + rect.height - border_width.bottom,
            .width = rect.width,
            .height = border_width.bottom,
        },
        color);
  }

  if (border_width.left > 0)
  {
    canvas.FillRect({.x = rect.x, .y = rect.y, .width = border_width.left, .height = rect.height},
                    color);
  }
}

void DrawImagePlaceholder(platform::window::Canvas& canvas,
                          platform::window::Rect rect,
                          platform::window::Color color)
{
  const platform::window::Color border{.red = 92, .green = 92, .blue = 92, .alpha = 255};
  const platform::window::Color text{.red = 52, .green = 52, .blue = 52, .alpha = 255};

  canvas.FillRect(rect, {.red = color.red, .green = color.green, .blue = color.blue, .alpha = 80});
  canvas.StrokeRect(rect, border, 1);
  canvas.DrawLine(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height, border, 1);
  canvas.DrawLine(rect.x + rect.width, rect.y, rect.x, rect.y + rect.height, border, 1);

  if (rect.width >= 28 && rect.height >= 18)
  {
    canvas.DrawText("IMG", rect.x + 4, rect.y + 2, std::min(12, rect.height - 4), text);
  }
}

} // namespace

void DisplayListRenderer::Render(const engine::paint::DisplayList& display_list,
                                 platform::window::Canvas& canvas,
                                 platform::window::Rect viewport,
                                 int scroll_y) const
{
  canvas.PushClip(viewport);
  for (const engine::paint::DisplayCommand& command : display_list.commands)
  {
    const platform::window::Rect rect = TranslateRect(command.rect, viewport, scroll_y);
    if (!Intersects(rect, viewport))
    {
      continue;
    }

    const platform::window::Color color = ToPlatformColor(command.color);
    switch (command.type)
    {
    case engine::paint::DisplayCommandType::kRect:
      canvas.FillRect(rect, color);
      break;
    case engine::paint::DisplayCommandType::kText:
      canvas.DrawText(command.text, rect.x, rect.y, command.font_size_px, color);
      break;
    case engine::paint::DisplayCommandType::kBorder:
      DrawBorder(canvas, rect, color, command.border_width);
      break;
    case engine::paint::DisplayCommandType::kImagePlaceholder:
      DrawImagePlaceholder(canvas, rect, color);
      break;
    }
  }
  canvas.PopClip();
}

} // namespace speed::ui::gui
