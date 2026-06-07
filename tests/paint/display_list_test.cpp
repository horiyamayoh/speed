#include "engine/render_pipeline.h"

#include <cassert>
#include <string_view>

namespace
{

const speed::engine::paint::DisplayCommand*
FindCommand(const speed::engine::paint::DisplayList& display_list,
            speed::engine::paint::DisplayCommandType type)
{
  for (const speed::engine::paint::DisplayCommand& command : display_list.commands)
  {
    if (command.type == type)
    {
      return &command;
    }
  }

  return nullptr;
}

const speed::engine::paint::DisplayCommand*
FindTextCommand(const speed::engine::paint::DisplayList& display_list, std::string_view text)
{
  for (const speed::engine::paint::DisplayCommand& command : display_list.commands)
  {
    if (command.type == speed::engine::paint::DisplayCommandType::kText && command.text == text)
    {
      return &command;
    }
  }

  return nullptr;
}

void PaintsMvpDisplayCommands()
{
  const speed::engine::RenderPipeline pipeline;
  const speed::engine::RenderResult result = pipeline.RenderHtml(
      "<html><head><style>"
      "#card { background-color: #102030; border: 3px solid red; color: white; font-size: 18px; }"
      "img { width: 20px; height: 10px; }"
      "</style></head><body><div id=card>Hi <img src=x></div></body></html>",
      {.width = 200, .height = 120});

  const speed::engine::paint::DisplayCommand* rect =
      FindCommand(result.display_list, speed::engine::paint::DisplayCommandType::kRect);
  assert(rect != nullptr);
  const speed::engine::style::Color card_background{
      .red = 16, .green = 32, .blue = 48, .alpha = 255};
  assert(rect->color == card_background);

  const speed::engine::paint::DisplayCommand* border =
      FindCommand(result.display_list, speed::engine::paint::DisplayCommandType::kBorder);
  assert(border != nullptr);
  assert(border->border_width.top == 3);
  assert(border->border_width.right == 3);
  assert(border->border_width.bottom == 3);
  assert(border->border_width.left == 3);
  const speed::engine::style::Color red{.red = 255, .green = 0, .blue = 0, .alpha = 255};
  assert(border->color == red);

  const speed::engine::paint::DisplayCommand* text = FindTextCommand(result.display_list, "Hi");
  assert(text != nullptr);
  assert(text->font_size_px == 18);
  const speed::engine::style::Color white{.red = 255, .green = 255, .blue = 255, .alpha = 255};
  assert(text->color == white);

  const speed::engine::paint::DisplayCommand* image =
      FindCommand(result.display_list, speed::engine::paint::DisplayCommandType::kImagePlaceholder);
  assert(image != nullptr);
  assert(image->rect.width == 20);
  assert(image->rect.height == 10);
}

} // namespace

int main()
{
  PaintsMvpDisplayCommands();

  return 0;
}
