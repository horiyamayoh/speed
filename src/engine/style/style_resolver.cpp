#include "engine/style/style_resolver.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

using speed::engine::dom::Node;
using speed::engine::dom::NodeType;

struct CascadeValue final
{
  bool set{false};
  speed::engine::css::Specificity specificity;
  std::size_t source_order{0};
  std::string value;
};

[[nodiscard]] bool IsAsciiWhitespace(char character)
{
  return character == ' ' || character == '\t' || character == '\n' || character == '\r' ||
         character == '\f' || character == '\v';
}

[[nodiscard]] char ToAsciiLower(char character)
{
  if (character >= 'A' && character <= 'Z')
  {
    return static_cast<char>(character - 'A' + 'a');
  }

  return character;
}

[[nodiscard]] std::string ToAsciiLower(std::string_view input)
{
  std::string output(input);
  for (char& character : output)
  {
    character = ToAsciiLower(character);
  }
  return output;
}

[[nodiscard]] std::string Trim(std::string_view input)
{
  std::size_t begin = 0;
  while (begin < input.size() && IsAsciiWhitespace(input[begin]))
  {
    ++begin;
  }

  std::size_t end = input.size();
  while (end > begin && IsAsciiWhitespace(input[end - 1]))
  {
    --end;
  }

  return std::string(input.substr(begin, end - begin));
}

[[nodiscard]] const std::string* AttributeValue(const Node& node, std::string_view name)
{
  for (const speed::engine::dom::Attribute& attribute : node.attributes)
  {
    if (attribute.name == name)
    {
      return &attribute.value;
    }
  }
  return nullptr;
}

[[nodiscard]] bool HasClass(const Node& node, std::string_view class_name)
{
  const std::string* classes = AttributeValue(node, "class");
  if (classes == nullptr)
  {
    return false;
  }

  std::size_t position = 0;
  while (position < classes->size())
  {
    while (position < classes->size() && IsAsciiWhitespace((*classes)[position]))
    {
      ++position;
    }

    const std::size_t begin = position;
    while (position < classes->size() && !IsAsciiWhitespace((*classes)[position]))
    {
      ++position;
    }

    if (std::string_view(*classes).substr(begin, position - begin) == class_name)
    {
      return true;
    }
  }

  return false;
}

[[nodiscard]] bool MatchesSimpleSelector(const speed::engine::css::SimpleSelector& selector,
                                         const Node& node)
{
  if (node.type != NodeType::kElement)
  {
    return false;
  }

  if (!selector.type.empty() && selector.type != node.name)
  {
    return false;
  }

  if (!selector.id.empty())
  {
    const std::string* id = AttributeValue(node, "id");
    if (id == nullptr || *id != selector.id)
    {
      return false;
    }
  }

  return std::ranges::all_of(selector.classes,
                             [&node](const std::string& class_name)
                             { return HasClass(node, class_name); });
}

[[nodiscard]] bool MatchesSelector(const speed::engine::css::Selector& selector,
                                   const Node& node,
                                   const std::vector<const Node*>& ancestors)
{
  if (selector.parts.empty())
  {
    return false;
  }

  std::size_t part_index = selector.parts.size() - 1;
  if (!MatchesSimpleSelector(selector.parts[part_index], node))
  {
    return false;
  }

  std::size_t ancestor_count = ancestors.size();
  while (part_index > 0)
  {
    --part_index;
    bool found = false;
    while (ancestor_count > 0)
    {
      --ancestor_count;
      if (MatchesSimpleSelector(selector.parts[part_index], *ancestors[ancestor_count]))
      {
        found = true;
        break;
      }
    }

    if (!found)
    {
      return false;
    }
  }

  return true;
}

[[nodiscard]] bool ShouldReplace(const CascadeValue& current,
                                 const speed::engine::css::Specificity& specificity,
                                 std::size_t source_order)
{
  if (!current.set)
  {
    return true;
  }

  if (current.specificity < specificity)
  {
    return true;
  }

  return current.specificity == specificity && current.source_order <= source_order;
}

void ApplyCascadeDeclaration(std::unordered_map<std::string, CascadeValue>& cascade,
                             const speed::engine::css::Declaration& declaration,
                             const speed::engine::css::Specificity& specificity,
                             std::size_t source_order)
{
  CascadeValue& current = cascade[declaration.property];
  if (!ShouldReplace(current, specificity, source_order))
  {
    return;
  }

  current = {
      .set = true,
      .specificity = specificity,
      .source_order = source_order,
      .value = declaration.value,
  };
}

void AppendTextContent(const Node& node, std::string& output)
{
  if (node.type == NodeType::kText)
  {
    output += node.text;
    return;
  }

  for (const Node& child : node.children)
  {
    AppendTextContent(child, output);
  }
}

void CollectStyleText(const Node& node, std::vector<std::string>& style_blocks)
{
  if (node.type == NodeType::kElement && node.name == "style")
  {
    std::string text;
    AppendTextContent(node, text);
    if (!text.empty())
    {
      style_blocks.push_back(std::move(text));
    }
    return;
  }

  for (const Node& child : node.children)
  {
    CollectStyleText(child, style_blocks);
  }
}

[[nodiscard]] speed::engine::css::StyleSheet BuildAuthorStyleSheet(const Node& document)
{
  std::vector<std::string> style_blocks;
  CollectStyleText(document, style_blocks);

  speed::engine::css::CssParser parser;
  speed::engine::css::StyleSheet sheet;
  for (const std::string& style_block : style_blocks)
  {
    speed::engine::css::StyleSheet parsed = parser.ParseStyleSheet(style_block);
    for (speed::engine::css::CssRule& rule : parsed.rules)
    {
      rule.source_order = sheet.rules.size();
      sheet.rules.push_back(std::move(rule));
    }
  }

  return sheet;
}

[[nodiscard]] std::optional<int> ParseInteger(std::string_view input)
{
  const std::string trimmed = Trim(input);
  if (trimmed.empty())
  {
    return std::nullopt;
  }

  int value = 0;
  const char* begin = trimmed.data();
  const char* end = trimmed.data() + trimmed.size();
  const std::from_chars_result result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end)
  {
    return std::nullopt;
  }
  return value;
}

[[nodiscard]] std::optional<speed::engine::style::Length> ParseLength(std::string_view value)
{
  const std::string lower = ToAsciiLower(Trim(value));
  if (lower == "auto")
  {
    return speed::engine::style::AutoLength();
  }

  std::string_view numeric = lower;
  if (lower.ends_with("px"))
  {
    numeric = std::string_view(lower).substr(0, lower.size() - 2);
  }

  const std::optional<int> pixels = ParseInteger(numeric);
  if (!pixels.has_value())
  {
    return std::nullopt;
  }

  return speed::engine::style::PxLength(*pixels);
}

[[nodiscard]] std::optional<speed::engine::style::Color> ParseHexColor(std::string_view value)
{
  if (!value.starts_with('#'))
  {
    return std::nullopt;
  }

  auto hex_value = [](char character) -> std::optional<std::uint8_t>
  {
    if (character >= '0' && character <= '9')
    {
      return static_cast<std::uint8_t>(character - '0');
    }
    if (character >= 'a' && character <= 'f')
    {
      return static_cast<std::uint8_t>(character - 'a' + 10);
    }
    if (character >= 'A' && character <= 'F')
    {
      return static_cast<std::uint8_t>(character - 'A' + 10);
    }
    return std::nullopt;
  };

  if (value.size() == 4)
  {
    const std::optional<std::uint8_t> red = hex_value(value[1]);
    const std::optional<std::uint8_t> green = hex_value(value[2]);
    const std::optional<std::uint8_t> blue = hex_value(value[3]);
    if (!red.has_value() || !green.has_value() || !blue.has_value())
    {
      return std::nullopt;
    }

    return speed::engine::style::Color{
        .red = static_cast<std::uint8_t>(*red * 17),
        .green = static_cast<std::uint8_t>(*green * 17),
        .blue = static_cast<std::uint8_t>(*blue * 17),
        .alpha = 255,
    };
  }

  if (value.size() != 7)
  {
    return std::nullopt;
  }

  const std::optional<std::uint8_t> red_high = hex_value(value[1]);
  const std::optional<std::uint8_t> red_low = hex_value(value[2]);
  const std::optional<std::uint8_t> green_high = hex_value(value[3]);
  const std::optional<std::uint8_t> green_low = hex_value(value[4]);
  const std::optional<std::uint8_t> blue_high = hex_value(value[5]);
  const std::optional<std::uint8_t> blue_low = hex_value(value[6]);
  if (!red_high.has_value() || !red_low.has_value() || !green_high.has_value() ||
      !green_low.has_value() || !blue_high.has_value() || !blue_low.has_value())
  {
    return std::nullopt;
  }

  return speed::engine::style::Color{
      .red = static_cast<std::uint8_t>((*red_high * 16) + *red_low),
      .green = static_cast<std::uint8_t>((*green_high * 16) + *green_low),
      .blue = static_cast<std::uint8_t>((*blue_high * 16) + *blue_low),
      .alpha = 255,
  };
}

[[nodiscard]] std::optional<speed::engine::style::Color> ParseColor(std::string_view value)
{
  const std::string lower = ToAsciiLower(Trim(value));
  if (lower == "transparent")
  {
    return speed::engine::style::Color{.red = 0, .green = 0, .blue = 0, .alpha = 0};
  }
  if (lower == "black")
  {
    return speed::engine::style::Color{.red = 0, .green = 0, .blue = 0, .alpha = 255};
  }
  if (lower == "white")
  {
    return speed::engine::style::Color{.red = 255, .green = 255, .blue = 255, .alpha = 255};
  }
  if (lower == "red")
  {
    return speed::engine::style::Color{.red = 255, .green = 0, .blue = 0, .alpha = 255};
  }
  if (lower == "green")
  {
    return speed::engine::style::Color{.red = 0, .green = 128, .blue = 0, .alpha = 255};
  }
  if (lower == "blue")
  {
    return speed::engine::style::Color{.red = 0, .green = 0, .blue = 255, .alpha = 255};
  }
  if (lower == "gray" || lower == "grey")
  {
    return speed::engine::style::Color{.red = 128, .green = 128, .blue = 128, .alpha = 255};
  }

  return ParseHexColor(lower);
}

[[nodiscard]] speed::engine::style::Display DefaultDisplayForNode(const Node& node)
{
  if (node.type == NodeType::kText)
  {
    return speed::engine::style::Display::kInline;
  }

  if (node.type == NodeType::kDocument)
  {
    return speed::engine::style::Display::kBlock;
  }

  if (speed::engine::style::IsLayoutSuppressedElement(node.name))
  {
    return speed::engine::style::Display::kNone;
  }

  if (node.name == "span" || node.name == "a" || node.name == "br" || node.name == "img")
  {
    return speed::engine::style::Display::kInline;
  }

  return speed::engine::style::Display::kBlock;
}

[[nodiscard]] speed::engine::style::ComputedStyle
InitialStyleForNode(const Node& node, const speed::engine::style::ComputedStyle* parent_style)
{
  speed::engine::style::ComputedStyle style;
  style.display = DefaultDisplayForNode(node);
  style.color = parent_style == nullptr
                    ? speed::engine::style::Color{.red = 0, .green = 0, .blue = 0, .alpha = 255}
                    : parent_style->color;
  style.font_size_px = parent_style == nullptr ? 16 : parent_style->font_size_px;
  style.border_color = style.color;
  return style;
}

void ApplyProperty(speed::engine::style::ComputedStyle& style,
                   std::string_view property,
                   std::string_view value)
{
  const std::string lower_value = ToAsciiLower(Trim(value));

  if (property == "display")
  {
    if (lower_value == "block")
    {
      style.display = speed::engine::style::Display::kBlock;
    }
    else if (lower_value == "inline")
    {
      style.display = speed::engine::style::Display::kInline;
    }
    else if (lower_value == "none")
    {
      style.display = speed::engine::style::Display::kNone;
    }
    return;
  }

  if (property == "width")
  {
    if (const std::optional<speed::engine::style::Length> length = ParseLength(value);
        length.has_value())
    {
      style.width = *length;
    }
    return;
  }

  if (property == "height")
  {
    if (const std::optional<speed::engine::style::Length> length = ParseLength(value);
        length.has_value())
    {
      style.height = *length;
    }
    return;
  }

  if (property == "font-size")
  {
    if (lower_value == "inherit")
    {
      return;
    }
    if (const std::optional<speed::engine::style::Length> length = ParseLength(value);
        length.has_value() && length->unit == speed::engine::style::LengthUnit::kPx)
    {
      style.font_size_px = std::max(1, length->pixels);
    }
    return;
  }

  if (property == "color")
  {
    if (lower_value == "inherit")
    {
      return;
    }
    if (const std::optional<speed::engine::style::Color> color = ParseColor(value);
        color.has_value())
    {
      style.color = *color;
      style.border_color = *color;
    }
    return;
  }

  if (property == "background-color")
  {
    if (const std::optional<speed::engine::style::Color> color = ParseColor(value);
        color.has_value())
    {
      style.background_color = *color;
    }
    return;
  }

  if (property == "border-color")
  {
    if (const std::optional<speed::engine::style::Color> color = ParseColor(value);
        color.has_value())
    {
      style.border_color = *color;
    }
    return;
  }

  auto apply_edge = [&value](int& edge)
  {
    if (const std::optional<speed::engine::style::Length> length = ParseLength(value);
        length.has_value() && length->unit == speed::engine::style::LengthUnit::kPx)
    {
      edge = std::max(0, length->pixels);
    }
  };

  if (property == "margin-top")
  {
    apply_edge(style.margin.top);
  }
  else if (property == "margin-right")
  {
    apply_edge(style.margin.right);
  }
  else if (property == "margin-bottom")
  {
    apply_edge(style.margin.bottom);
  }
  else if (property == "margin-left")
  {
    apply_edge(style.margin.left);
  }
  else if (property == "padding-top")
  {
    apply_edge(style.padding.top);
  }
  else if (property == "padding-right")
  {
    apply_edge(style.padding.right);
  }
  else if (property == "padding-bottom")
  {
    apply_edge(style.padding.bottom);
  }
  else if (property == "padding-left")
  {
    apply_edge(style.padding.left);
  }
  else if (property == "border-top-width")
  {
    apply_edge(style.border_width.top);
  }
  else if (property == "border-right-width")
  {
    apply_edge(style.border_width.right);
  }
  else if (property == "border-bottom-width")
  {
    apply_edge(style.border_width.bottom);
  }
  else if (property == "border-left-width")
  {
    apply_edge(style.border_width.left);
  }
}

void ApplyCascade(speed::engine::style::ComputedStyle& style,
                  const std::unordered_map<std::string, CascadeValue>& cascade)
{
  const std::vector<std::string_view> property_order = {
      "display",
      "width",
      "height",
      "font-size",
      "color",
      "background-color",
      "border-color",
      "margin-top",
      "margin-right",
      "margin-bottom",
      "margin-left",
      "padding-top",
      "padding-right",
      "padding-bottom",
      "padding-left",
      "border-top-width",
      "border-right-width",
      "border-bottom-width",
      "border-left-width",
  };

  for (std::string_view property : property_order)
  {
    const auto found = cascade.find(std::string(property));
    if (found != cascade.end())
    {
      ApplyProperty(style, property, found->second.value);
    }
  }
}

[[nodiscard]] speed::engine::style::ComputedStyle
ResolveStyleForNode(const Node& node,
                    const speed::engine::style::ComputedStyle* parent_style,
                    const speed::engine::css::StyleSheet& stylesheet,
                    const std::vector<const Node*>& ancestors)
{
  speed::engine::style::ComputedStyle style = InitialStyleForNode(node, parent_style);
  if (node.type != NodeType::kElement)
  {
    return style;
  }

  std::unordered_map<std::string, CascadeValue> cascade;
  std::size_t source_order = 0;
  for (const speed::engine::css::CssRule& rule : stylesheet.rules)
  {
    const speed::engine::css::Specificity specificity =
        speed::engine::css::CalculateSpecificity(rule.selector);
    if (!MatchesSelector(rule.selector, node, ancestors))
    {
      source_order += rule.declarations.size();
      continue;
    }

    for (const speed::engine::css::Declaration& declaration : rule.declarations)
    {
      ApplyCascadeDeclaration(cascade, declaration, specificity, source_order);
      ++source_order;
    }
  }

  if (const std::string* inline_style = AttributeValue(node, "style"); inline_style != nullptr)
  {
    speed::engine::css::CssParser parser;
    const speed::engine::css::Specificity inline_specificity{.ids = 1000, .classes = 0, .types = 0};
    for (const speed::engine::css::Declaration& declaration :
         parser.ParseDeclarationList(*inline_style))
    {
      ApplyCascadeDeclaration(cascade, declaration, inline_specificity, source_order);
      ++source_order;
    }
  }

  ApplyCascade(style, cascade);
  if (speed::engine::style::IsLayoutSuppressedElement(node.name))
  {
    style.display = speed::engine::style::Display::kNone;
  }
  return style;
}

[[nodiscard]] speed::engine::style::StyledNode
ResolveNode(const Node& node,
            const speed::engine::style::ComputedStyle* parent_style,
            const speed::engine::css::StyleSheet& stylesheet,
            std::vector<const Node*>& ancestors)
{
  speed::engine::style::StyledNode styled;
  styled.node = &node;
  styled.style = ResolveStyleForNode(node, parent_style, stylesheet, ancestors);

  const bool is_element = node.type == NodeType::kElement;
  if (is_element)
  {
    ancestors.push_back(&node);
  }

  for (const Node& child : node.children)
  {
    styled.children.push_back(ResolveNode(child, &styled.style, stylesheet, ancestors));
  }

  if (is_element)
  {
    ancestors.pop_back();
  }

  return styled;
}

} // namespace

namespace speed::engine::style
{

bool operator==(const Length& left, const Length& right)
{
  return left.unit == right.unit && left.pixels == right.pixels;
}

bool operator==(const Color& left, const Color& right)
{
  return left.red == right.red && left.green == right.green && left.blue == right.blue &&
         left.alpha == right.alpha;
}

bool operator==(const Edges& left, const Edges& right)
{
  return left.top == right.top && left.right == right.right && left.bottom == right.bottom &&
         left.left == right.left;
}

Length AutoLength()
{
  return {.unit = LengthUnit::kAuto, .pixels = 0};
}

Length PxLength(int pixels)
{
  return {.unit = LengthUnit::kPx, .pixels = pixels};
}

bool IsTransparent(Color color)
{
  return color.alpha == 0;
}

bool IsLayoutSuppressedElement(std::string_view element_name)
{
  return element_name == "head" || element_name == "style";
}

StyledNode StyleResolver::Resolve(const dom::Node& document) const
{
  const css::StyleSheet stylesheet = BuildAuthorStyleSheet(document);
  std::vector<const dom::Node*> ancestors;
  return ResolveNode(document, nullptr, stylesheet, ancestors);
}

} // namespace speed::engine::style
