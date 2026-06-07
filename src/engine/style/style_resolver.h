#pragma once

#include "engine/css/css_parser.h"
#include "engine/dom/node.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace speed::engine::style
{

enum class Display : std::uint8_t
{
  kBlock,
  kInline,
  kNone,
};

enum class LengthUnit : std::uint8_t
{
  kAuto,
  kPx,
};

struct Length final
{
  LengthUnit unit{LengthUnit::kAuto};
  int pixels{0};
};

struct Color final
{
  std::uint8_t red{0};
  std::uint8_t green{0};
  std::uint8_t blue{0};
  std::uint8_t alpha{255};
};

struct Edges final
{
  int top{0};
  int right{0};
  int bottom{0};
  int left{0};
};

struct ComputedStyle final
{
  Display display{Display::kBlock};
  Length width{};
  Length height{};
  Edges margin{};
  Edges padding{};
  Edges border_width{};
  Color color{};
  Color background_color{.red = 0, .green = 0, .blue = 0, .alpha = 0};
  Color border_color{};
  int font_size_px{16};
};

struct StyledNode final
{
  const dom::Node* node{nullptr};
  ComputedStyle style;
  std::vector<StyledNode> children;
};

[[nodiscard]] bool operator==(const Length& left, const Length& right);
[[nodiscard]] bool operator==(const Color& left, const Color& right);
[[nodiscard]] bool operator==(const Edges& left, const Edges& right);
[[nodiscard]] Length AutoLength();
[[nodiscard]] Length PxLength(int pixels);
[[nodiscard]] bool IsTransparent(Color color);
[[nodiscard]] bool IsLayoutSuppressedElement(std::string_view element_name);

class StyleResolver final
{
public:
  [[nodiscard]] StyledNode Resolve(const dom::Node& document) const;
};

} // namespace speed::engine::style
