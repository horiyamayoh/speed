#pragma once

#include "engine/layout/layout_engine.h"
#include "engine/style/style_resolver.h"

#include <cstdint>
#include <string>
#include <vector>

namespace speed::engine::paint
{

enum class DisplayCommandType : std::uint8_t
{
  kRect,
  kText,
  kBorder,
  kImagePlaceholder,
};

struct DisplayCommand final
{
  DisplayCommandType type{DisplayCommandType::kRect};
  layout::Rect rect;
  style::Color color;
  style::Edges border_width;
  int font_size_px{16};
  std::string text;
};

struct DisplayList final
{
  std::vector<DisplayCommand> commands;
};

class Painter final
{
public:
  [[nodiscard]] DisplayList Paint(const layout::LayoutResult& layout) const;
};

} // namespace speed::engine::paint
