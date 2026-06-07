#include "ui/gui/display_list_renderer.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace
{

enum class OperationType : std::uint8_t
{
  kFillRect,
  kStrokeRect,
  kDrawLine,
  kDrawText,
  kPushClip,
  kPopClip,
};

struct Operation final
{
  OperationType type{OperationType::kFillRect};
  speed::platform::window::Rect rect;
  int x1{0};
  int y1{0};
  int x2{0};
  int y2{0};
  int stroke_width{0};
  int font_size_px{0};
  std::string text;
};

class RecordingCanvas final : public speed::platform::window::Canvas
{
public:
  void FillRect(speed::platform::window::Rect rect,
                speed::platform::window::Color /*color*/) override
  {
    Operation operation;
    operation.type = OperationType::kFillRect;
    operation.rect = rect;
    operations.push_back(std::move(operation));
  }

  void StrokeRect(speed::platform::window::Rect rect,
                  speed::platform::window::Color /*color*/,
                  int stroke_width) override
  {
    Operation operation;
    operation.type = OperationType::kStrokeRect;
    operation.rect = rect;
    operation.stroke_width = stroke_width;
    operations.push_back(std::move(operation));
  }

  void DrawLine(int x1,
                int y1,
                int x2,
                int y2,
                speed::platform::window::Color /*color*/,
                int stroke_width) override
  {
    Operation operation;
    operation.type = OperationType::kDrawLine;
    operation.x1 = x1;
    operation.y1 = y1;
    operation.x2 = x2;
    operation.y2 = y2;
    operation.stroke_width = stroke_width;
    operations.push_back(std::move(operation));
  }

  void DrawText(std::string_view text,
                int x,
                int y,
                int font_size_px,
                speed::platform::window::Color /*color*/) override
  {
    Operation operation;
    operation.type = OperationType::kDrawText;
    operation.x1 = x;
    operation.y1 = y;
    operation.font_size_px = font_size_px;
    operation.text = std::string(text);
    operations.push_back(std::move(operation));
  }

  void PushClip(speed::platform::window::Rect rect) override
  {
    Operation operation;
    operation.type = OperationType::kPushClip;
    operation.rect = rect;
    operations.push_back(std::move(operation));
  }

  void PopClip() override
  {
    Operation operation;
    operation.type = OperationType::kPopClip;
    operations.push_back(std::move(operation));
  }

  std::vector<Operation> operations;
};

[[nodiscard]] bool HasFillAt(const RecordingCanvas& canvas, speed::platform::window::Rect expected)
{
  for (const Operation& operation : canvas.operations)
  {
    if (operation.type == OperationType::kFillRect && operation.rect.x == expected.x &&
        operation.rect.y == expected.y && operation.rect.width == expected.width &&
        operation.rect.height == expected.height)
    {
      return true;
    }
  }

  return false;
}

[[nodiscard]] bool HasTextAt(const RecordingCanvas& canvas, std::string_view text, int x, int y)
{
  for (const Operation& operation : canvas.operations)
  {
    if (operation.type == OperationType::kDrawText && operation.text == text && operation.x1 == x &&
        operation.y1 == y)
    {
      return true;
    }
  }

  return false;
}

[[nodiscard]] int CountOperations(const RecordingCanvas& canvas, OperationType type)
{
  int count = 0;
  for (const Operation& operation : canvas.operations)
  {
    if (operation.type == type)
    {
      ++count;
    }
  }
  return count;
}

void RendersDisplayListToCanvas()
{
  speed::engine::paint::DisplayList display_list;

  speed::engine::paint::DisplayCommand rect;
  rect.type = speed::engine::paint::DisplayCommandType::kRect;
  rect.rect = {.x = 4, .y = 20, .width = 40, .height = 18};
  rect.color = {.red = 16, .green = 32, .blue = 48, .alpha = 255};
  display_list.commands.push_back(std::move(rect));

  speed::engine::paint::DisplayCommand text;
  text.type = speed::engine::paint::DisplayCommandType::kText;
  text.rect = {.x = 6, .y = 40, .width = 50, .height = 20};
  text.color = {.red = 255, .green = 255, .blue = 255, .alpha = 255};
  text.font_size_px = 14;
  text.text = "Speed";
  display_list.commands.push_back(std::move(text));

  speed::engine::paint::DisplayCommand border;
  border.type = speed::engine::paint::DisplayCommandType::kBorder;
  border.rect = {.x = 8, .y = 64, .width = 80, .height = 30};
  border.color = {.red = 255, .green = 0, .blue = 0, .alpha = 255};
  border.border_width = {.top = 2, .right = 3, .bottom = 4, .left = 5};
  display_list.commands.push_back(std::move(border));

  speed::engine::paint::DisplayCommand image;
  image.type = speed::engine::paint::DisplayCommandType::kImagePlaceholder;
  image.rect = {.x = 12, .y = 100, .width = 64, .height = 48};
  image.color = {.red = 160, .green = 160, .blue = 160, .alpha = 255};
  display_list.commands.push_back(std::move(image));

  RecordingCanvas canvas;
  const speed::ui::gui::DisplayListRenderer renderer;
  renderer.Render(display_list, canvas, {.x = 10, .y = 20, .width = 160, .height = 140}, 15);

  assert(!canvas.operations.empty());
  assert(canvas.operations.front().type == OperationType::kPushClip);
  assert(canvas.operations.back().type == OperationType::kPopClip);
  assert(HasFillAt(canvas, {.x = 14, .y = 25, .width = 40, .height = 18}));
  assert(HasTextAt(canvas, "Speed", 16, 45));
  assert(HasFillAt(canvas, {.x = 18, .y = 69, .width = 80, .height = 2}));
  assert(HasFillAt(canvas, {.x = 95, .y = 69, .width = 3, .height = 30}));
  assert(CountOperations(canvas, OperationType::kStrokeRect) >= 1);
  assert(CountOperations(canvas, OperationType::kDrawLine) >= 2);
  assert(HasTextAt(canvas, "IMG", 26, 107));
}

} // namespace

int main()
{
  RendersDisplayListToCanvas();
  return 0;
}
