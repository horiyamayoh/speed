#include "engine/layout/layout_engine.h"

#include "engine/dom/node.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

using speed::engine::dom::NodeType;
using speed::engine::layout::LayoutBox;
using speed::engine::layout::LayoutBoxType;
using speed::engine::layout::Rect;
using speed::engine::style::Display;
using speed::engine::style::LengthUnit;
using speed::engine::style::StyledNode;

constexpr int kDefaultImageWidth = 64;
constexpr int kDefaultImageHeight = 48;

struct InlineState final
{
  int content_x{0};
  int content_y{0};
  int max_width{0};
  int cursor_x{0};
  int line_y{0};
  int line_height{0};
  bool pending_space{false};
  std::vector<LayoutBox>* children{nullptr};
};

[[nodiscard]] int HorizontalEdges(const speed::engine::style::Edges& edges)
{
  return edges.left + edges.right;
}

[[nodiscard]] int VerticalEdges(const speed::engine::style::Edges& edges)
{
  return edges.top + edges.bottom;
}

[[nodiscard]] int ClampedNonNegative(int value)
{
  return std::max(0, value);
}

[[nodiscard]] std::string NodeNameFor(const StyledNode& node)
{
  if (node.node == nullptr)
  {
    return {};
  }
  return node.node->name;
}

[[nodiscard]] bool IsTextNode(const StyledNode& node)
{
  return node.node != nullptr && node.node->type == NodeType::kText;
}

[[nodiscard]] bool IsElement(const StyledNode& node, std::string_view name)
{
  return node.node != nullptr && node.node->type == NodeType::kElement && node.node->name == name;
}

[[nodiscard]] bool ShouldGenerateLayout(const StyledNode& node)
{
  if (node.style.display == Display::kNone)
  {
    return false;
  }

  return node.node == nullptr || !speed::engine::style::IsLayoutSuppressedElement(node.node->name);
}

[[nodiscard]] int UsedLengthOr(const speed::engine::style::Length& length, int fallback)
{
  if (length.unit == LengthUnit::kPx)
  {
    return ClampedNonNegative(length.pixels);
  }
  return fallback;
}

[[nodiscard]] int TextCharacterWidth(const speed::engine::style::ComputedStyle& style)
{
  return std::max(1, style.font_size_px / 2);
}

[[nodiscard]] int LineHeight(const speed::engine::style::ComputedStyle& style)
{
  return std::max(1, style.font_size_px + 4);
}

[[nodiscard]] int TextWidth(std::string_view text, const speed::engine::style::ComputedStyle& style)
{
  return static_cast<int>(text.size()) * TextCharacterWidth(style);
}

void StartNewLine(InlineState& state)
{
  state.cursor_x = state.content_x;
  state.line_y += std::max(1, state.line_height);
  state.line_height = 0;
  state.pending_space = false;
}

void ForceLineBreak(InlineState& state, const speed::engine::style::ComputedStyle& style)
{
  state.line_height = std::max(state.line_height, LineHeight(style));
  StartNewLine(state);
}

void AppendInlineBox(InlineState& state, LayoutBox box)
{
  if (box.rect.width <= 0 || box.rect.height <= 0)
  {
    return;
  }

  const int line_limit = state.content_x + state.max_width;
  if (state.cursor_x > state.content_x && state.cursor_x + box.rect.width > line_limit)
  {
    StartNewLine(state);
  }

  box.rect.x = state.cursor_x;
  box.rect.y = state.line_y;
  state.cursor_x += box.rect.width;
  state.line_height = std::max(state.line_height, box.rect.height);
  state.children->push_back(std::move(box));
}

void AppendTextRun(InlineState& state,
                   const StyledNode& node,
                   std::string text,
                   const speed::engine::style::ComputedStyle& style)
{
  if (text.empty())
  {
    return;
  }

  LayoutBox box;
  box.type = LayoutBoxType::kText;
  box.node_name = NodeNameFor(node);
  box.text = std::move(text);
  box.style = style;
  box.rect = {
      .x = 0,
      .y = 0,
      .width = TextWidth(box.text, style),
      .height = LineHeight(style),
  };
  AppendInlineBox(state, std::move(box));
}

void FlushPendingSpace(InlineState& state,
                       const StyledNode& node,
                       const speed::engine::style::ComputedStyle& style)
{
  if (!state.pending_space || state.cursor_x == state.content_x)
  {
    state.pending_space = false;
    return;
  }

  AppendTextRun(state, node, " ", style);
  state.pending_space = false;
}

void LayoutInlineText(InlineState& state, const StyledNode& node)
{
  if (node.node == nullptr)
  {
    return;
  }

  std::string word;
  for (const char character : node.node->text)
  {
    if (character == ' ' || character == '\t' || character == '\n' || character == '\r' ||
        character == '\f' || character == '\v')
    {
      if (!word.empty())
      {
        FlushPendingSpace(state, node, node.style);
        AppendTextRun(state, node, std::move(word), node.style);
        word.clear();
      }
      state.pending_space = true;
      continue;
    }

    word.push_back(character);
  }

  if (!word.empty())
  {
    FlushPendingSpace(state, node, node.style);
    AppendTextRun(state, node, std::move(word), node.style);
  }
}

void LayoutInlineNode(InlineState& state, const StyledNode& node);

void LayoutInlineChildren(InlineState& state, const StyledNode& node)
{
  for (const StyledNode& child : node.children)
  {
    LayoutInlineNode(state, child);
  }
}

void LayoutInlineImage(InlineState& state, const StyledNode& node)
{
  FlushPendingSpace(state, node, node.style);

  LayoutBox box;
  box.type = LayoutBoxType::kImagePlaceholder;
  box.node_name = NodeNameFor(node);
  box.style = node.style;
  box.rect = {
      .x = 0,
      .y = 0,
      .width = UsedLengthOr(node.style.width, kDefaultImageWidth),
      .height = UsedLengthOr(node.style.height, kDefaultImageHeight),
  };
  AppendInlineBox(state, std::move(box));
}

void LayoutInlineNode(InlineState& state, const StyledNode& node)
{
  if (!ShouldGenerateLayout(node))
  {
    return;
  }

  if (IsTextNode(node))
  {
    LayoutInlineText(state, node);
    return;
  }

  if (IsElement(node, "br"))
  {
    ForceLineBreak(state, node.style);
    return;
  }

  if (IsElement(node, "img"))
  {
    LayoutInlineImage(state, node);
    return;
  }

  LayoutInlineChildren(state, node);
}

[[nodiscard]] int FinishInlineLayout(InlineState& state)
{
  state.pending_space = false;
  if (state.line_height == 0)
  {
    return state.line_y - state.content_y;
  }
  return (state.line_y + state.line_height) - state.content_y;
}

[[nodiscard]] LayoutBox
LayoutBlockNode(const StyledNode& node, int containing_x, int border_box_y, int containing_width);

void FlushInlineStateIntoCursor(InlineState& inline_state, int& cursor_y)
{
  const int inline_height = FinishInlineLayout(inline_state);
  if (inline_height > 0)
  {
    cursor_y = inline_state.content_y + inline_height;
    inline_state.content_y = cursor_y;
    inline_state.line_y = cursor_y;
    inline_state.cursor_x = inline_state.content_x;
    inline_state.line_height = 0;
  }
}

void LayoutChildIntoBlock(LayoutBox& box,
                          InlineState& inline_state,
                          int& cursor_y,
                          const StyledNode& child,
                          int content_x,
                          int content_width)
{
  if (!ShouldGenerateLayout(child))
  {
    return;
  }

  if (child.style.display == Display::kBlock)
  {
    FlushInlineStateIntoCursor(inline_state, cursor_y);
    LayoutBox child_box = LayoutBlockNode(child, content_x, cursor_y, content_width);
    cursor_y = child_box.rect.y + child_box.rect.height + child.style.margin.bottom;
    box.children.push_back(std::move(child_box));
    inline_state.content_y = cursor_y;
    inline_state.line_y = cursor_y;
    inline_state.cursor_x = inline_state.content_x;
    return;
  }

  LayoutInlineNode(inline_state, child);
}

[[nodiscard]] LayoutBox
LayoutBlockImage(const StyledNode& node, int containing_x, int border_box_y, int containing_width)
{
  const speed::engine::style::ComputedStyle& style = node.style;
  const int available_width =
      ClampedNonNegative(containing_width - style.margin.left - style.margin.right);
  const int content_width =
      UsedLengthOr(style.width, std::min(kDefaultImageWidth, available_width));
  const int content_height = UsedLengthOr(style.height, kDefaultImageHeight);

  LayoutBox box;
  box.type = LayoutBoxType::kImagePlaceholder;
  box.node_name = NodeNameFor(node);
  box.style = style;
  box.rect = {
      .x = containing_x + style.margin.left,
      .y = border_box_y + style.margin.top,
      .width = content_width + HorizontalEdges(style.padding) + HorizontalEdges(style.border_width),
      .height = content_height + VerticalEdges(style.padding) + VerticalEdges(style.border_width),
  };
  return box;
}

[[nodiscard]] LayoutBox
LayoutBlockNode(const StyledNode& node, int containing_x, int border_box_y, int containing_width)
{
  if (IsElement(node, "img"))
  {
    return LayoutBlockImage(node, containing_x, border_box_y, containing_width);
  }

  const speed::engine::style::ComputedStyle& style = node.style;
  const int available_width =
      ClampedNonNegative(containing_width - style.margin.left - style.margin.right);
  const int non_content_width =
      HorizontalEdges(style.padding) + HorizontalEdges(style.border_width);
  const int content_width =
      UsedLengthOr(style.width, ClampedNonNegative(available_width - non_content_width));
  const int border_box_width = content_width + non_content_width;
  const int content_x =
      containing_x + style.margin.left + style.border_width.left + style.padding.left;
  const int content_y =
      border_box_y + style.margin.top + style.border_width.top + style.padding.top;

  LayoutBox box;
  box.type = LayoutBoxType::kBlock;
  box.node_name = NodeNameFor(node);
  box.style = style;
  box.rect = {
      .x = containing_x + style.margin.left,
      .y = border_box_y + style.margin.top,
      .width = border_box_width,
      .height = 0,
  };

  int cursor_y = content_y;
  InlineState inline_state{
      .content_x = content_x,
      .content_y = content_y,
      .max_width = content_width,
      .cursor_x = content_x,
      .line_y = content_y,
      .line_height = 0,
      .pending_space = false,
      .children = &box.children,
  };

  for (const StyledNode& child : node.children)
  {
    LayoutChildIntoBlock(box, inline_state, cursor_y, child, content_x, content_width);
  }
  FlushInlineStateIntoCursor(inline_state, cursor_y);

  const int auto_content_height = cursor_y - content_y;
  const int content_height = UsedLengthOr(style.height, auto_content_height);
  box.rect.height =
      content_height + VerticalEdges(style.padding) + VerticalEdges(style.border_width);
  return box;
}

} // namespace

namespace speed::engine::layout
{

LayoutResult LayoutEngine::Layout(const style::StyledNode& document, Viewport viewport) const
{
  viewport.width = std::max(1, viewport.width);
  viewport.height = std::max(1, viewport.height);

  LayoutResult result;
  result.root.type = LayoutBoxType::kViewport;
  result.root.node_name = "#viewport";
  result.root.rect = {
      .x = 0,
      .y = 0,
      .width = viewport.width,
      .height = viewport.height,
  };

  LayoutBox document_box = LayoutBlockNode(document, 0, 0, viewport.width);
  result.content_height = std::max(viewport.height, document_box.rect.height);
  result.root.children.push_back(std::move(document_box));
  return result;
}

} // namespace speed::engine::layout
