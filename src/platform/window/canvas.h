#pragma once

#include <cstdint>
#include <string_view>

namespace speed::platform::window
{

struct Color final
{
  std::uint8_t red{0};
  std::uint8_t green{0};
  std::uint8_t blue{0};
  std::uint8_t alpha{255};
};

struct Rect final
{
  int x{0};
  int y{0};
  int width{0};
  int height{0};
};

class Canvas
{
public:
  virtual ~Canvas() = default;

  virtual void FillRect(Rect rect, Color color) = 0;
  virtual void StrokeRect(Rect rect, Color color, int stroke_width) = 0;
  virtual void DrawLine(int x1, int y1, int x2, int y2, Color color, int stroke_width) = 0;
  virtual void DrawText(std::string_view text, int x, int y, int font_size_px, Color color) = 0;
  virtual void PushClip(Rect rect) = 0;
  virtual void PopClip() = 0;
};

} // namespace speed::platform::window
