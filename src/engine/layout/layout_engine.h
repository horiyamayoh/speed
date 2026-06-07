#pragma once

#include "engine/style/style_resolver.h"

#include <cstdint>
#include <string>
#include <vector>

namespace speed::engine::layout
{

struct Viewport final
{
  int width{800};
  int height{600};
};

struct Rect final
{
  int x{0};
  int y{0};
  int width{0};
  int height{0};
};

enum class LayoutBoxType : std::uint8_t
{
  kViewport,
  kBlock,
  kText,
  kImagePlaceholder,
};

struct LayoutBox final
{
  LayoutBoxType type{LayoutBoxType::kBlock};
  std::string node_name;
  std::string text;
  style::ComputedStyle style;
  Rect rect;
  std::vector<LayoutBox> children;
};

struct LayoutResult final
{
  LayoutBox root;
  int content_height{0};
};

class LayoutEngine final
{
public:
  [[nodiscard]] LayoutResult Layout(const style::StyledNode& document, Viewport viewport) const;
};

} // namespace speed::engine::layout
