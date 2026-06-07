#include "engine/css/css_parser.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

[[nodiscard]] bool IsAsciiWhitespace(char character)
{
  return character == ' ' || character == '\t' || character == '\n' || character == '\r' ||
         character == '\f' || character == '\v';
}

[[nodiscard]] bool IsAsciiAlpha(char character)
{
  return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

[[nodiscard]] bool IsAsciiDigit(char character)
{
  return character >= '0' && character <= '9';
}

[[nodiscard]] bool IsIdentifierCharacter(char character)
{
  return IsAsciiAlpha(character) || IsAsciiDigit(character) || character == '-' || character == '_';
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

[[nodiscard]] std::string StripComments(std::string_view input)
{
  std::string output;
  output.reserve(input.size());

  std::size_t position = 0;
  while (position < input.size())
  {
    if (position + 1 < input.size() && input[position] == '/' && input[position + 1] == '*')
    {
      const std::size_t close = input.find("*/", position + 2);
      position = close == std::string_view::npos ? input.size() : close + 2;
      continue;
    }

    output.push_back(input[position]);
    ++position;
  }

  return output;
}

[[nodiscard]] std::vector<std::string> SplitTopLevel(std::string_view input, char delimiter)
{
  std::vector<std::string> parts;
  std::size_t begin = 0;
  for (std::size_t position = 0; position <= input.size(); ++position)
  {
    if (position == input.size() || input[position] == delimiter)
    {
      std::string part = Trim(input.substr(begin, position - begin));
      if (!part.empty())
      {
        parts.push_back(std::move(part));
      }
      begin = position + 1;
    }
  }
  return parts;
}

[[nodiscard]] std::vector<std::string> SplitWhitespace(std::string_view input)
{
  std::vector<std::string> parts;
  std::size_t position = 0;
  while (position < input.size())
  {
    while (position < input.size() && IsAsciiWhitespace(input[position]))
    {
      ++position;
    }

    const std::size_t begin = position;
    while (position < input.size() && !IsAsciiWhitespace(input[position]))
    {
      ++position;
    }

    if (begin < position)
    {
      parts.emplace_back(input.substr(begin, position - begin));
    }
  }
  return parts;
}

[[nodiscard]] std::string ParseIdentifier(std::string_view input, std::size_t& position)
{
  const std::size_t begin = position;
  while (position < input.size() && IsIdentifierCharacter(input[position]))
  {
    ++position;
  }
  return std::string(input.substr(begin, position - begin));
}

[[nodiscard]] speed::engine::css::SimpleSelector ParseSimpleSelector(std::string_view token)
{
  speed::engine::css::SimpleSelector selector;
  std::size_t position = 0;

  if (position < token.size() && token[position] == '*')
  {
    ++position;
  }
  else if (position < token.size() && IsAsciiAlpha(token[position]))
  {
    selector.type = ToAsciiLower(ParseIdentifier(token, position));
  }

  while (position < token.size())
  {
    const char marker = token[position];
    if (marker != '.' && marker != '#')
    {
      ++position;
      continue;
    }

    ++position;
    std::string identifier = ParseIdentifier(token, position);
    if (identifier.empty())
    {
      continue;
    }

    if (marker == '.')
    {
      selector.classes.push_back(std::move(identifier));
    }
    else
    {
      selector.id = std::move(identifier);
    }
  }

  return selector;
}

[[nodiscard]] speed::engine::css::Selector ParseSelector(std::string_view input)
{
  speed::engine::css::Selector selector;
  for (const std::string& token : SplitWhitespace(input))
  {
    speed::engine::css::SimpleSelector simple_selector = ParseSimpleSelector(token);
    if (!simple_selector.type.empty() || !simple_selector.id.empty() ||
        !simple_selector.classes.empty())
    {
      selector.parts.push_back(std::move(simple_selector));
    }
  }
  return selector;
}

[[nodiscard]] std::vector<std::string> ExpandBoxValues(std::string_view value)
{
  std::vector<std::string> tokens = SplitWhitespace(value);
  if (tokens.empty())
  {
    return {};
  }

  if (tokens.size() == 1)
  {
    return {tokens[0], tokens[0], tokens[0], tokens[0]};
  }

  if (tokens.size() == 2)
  {
    return {tokens[0], tokens[1], tokens[0], tokens[1]};
  }

  if (tokens.size() == 3)
  {
    return {tokens[0], tokens[1], tokens[2], tokens[1]};
  }

  return {tokens[0], tokens[1], tokens[2], tokens[3]};
}

void AppendBoxDeclarations(std::vector<speed::engine::css::Declaration>& declarations,
                           std::string_view prefix,
                           std::string_view value)
{
  const std::vector<std::string> values = ExpandBoxValues(value);
  if (values.empty())
  {
    return;
  }

  declarations.push_back({.property = std::string(prefix) + "-top", .value = values[0]});
  declarations.push_back({.property = std::string(prefix) + "-right", .value = values[1]});
  declarations.push_back({.property = std::string(prefix) + "-bottom", .value = values[2]});
  declarations.push_back({.property = std::string(prefix) + "-left", .value = values[3]});
}

[[nodiscard]] bool LooksLikeLength(std::string_view token)
{
  if (token.empty())
  {
    return false;
  }

  std::size_t position = 0;
  if (token[position] == '+' || token[position] == '-')
  {
    ++position;
  }

  bool saw_digit = false;
  while (position < token.size() && IsAsciiDigit(token[position]))
  {
    saw_digit = true;
    ++position;
  }

  if (!saw_digit)
  {
    return false;
  }

  if (position == token.size())
  {
    return true;
  }

  return ToAsciiLower(token.substr(position)) == "px";
}

[[nodiscard]] bool IsBorderStyleKeyword(std::string_view token)
{
  const std::string lower = ToAsciiLower(token);
  return lower == "none" || lower == "solid" || lower == "dashed" || lower == "dotted" ||
         lower == "double";
}

void AppendBorderDeclarations(std::vector<speed::engine::css::Declaration>& declarations,
                              std::string_view value)
{
  std::string width;
  std::string color;

  for (const std::string& token : SplitWhitespace(value))
  {
    if (width.empty() && LooksLikeLength(token))
    {
      width = token;
      continue;
    }

    if (!IsBorderStyleKeyword(token))
    {
      color = token;
    }
  }

  if (!width.empty())
  {
    declarations.push_back({.property = "border-top-width", .value = width});
    declarations.push_back({.property = "border-right-width", .value = width});
    declarations.push_back({.property = "border-bottom-width", .value = width});
    declarations.push_back({.property = "border-left-width", .value = width});
  }

  if (!color.empty())
  {
    declarations.push_back({.property = "border-color", .value = color});
  }
}

void AppendExpandedDeclaration(std::vector<speed::engine::css::Declaration>& declarations,
                               std::string property,
                               std::string value)
{
  if (property == "margin")
  {
    AppendBoxDeclarations(declarations, "margin", value);
    return;
  }

  if (property == "padding")
  {
    AppendBoxDeclarations(declarations, "padding", value);
    return;
  }

  if (property == "border-width")
  {
    const std::vector<std::string> values = ExpandBoxValues(value);
    if (values.empty())
    {
      return;
    }
    declarations.push_back({.property = "border-top-width", .value = values[0]});
    declarations.push_back({.property = "border-right-width", .value = values[1]});
    declarations.push_back({.property = "border-bottom-width", .value = values[2]});
    declarations.push_back({.property = "border-left-width", .value = values[3]});
    return;
  }

  if (property == "border")
  {
    AppendBorderDeclarations(declarations, value);
    return;
  }

  declarations.push_back({.property = std::move(property), .value = std::move(value)});
}

} // namespace

namespace speed::engine::css
{

bool operator<(const Specificity& left, const Specificity& right)
{
  if (left.ids != right.ids)
  {
    return left.ids < right.ids;
  }

  if (left.classes != right.classes)
  {
    return left.classes < right.classes;
  }

  return left.types < right.types;
}

bool operator==(const Specificity& left, const Specificity& right)
{
  return left.ids == right.ids && left.classes == right.classes && left.types == right.types;
}

Specificity CalculateSpecificity(const Selector& selector)
{
  Specificity specificity;
  for (const SimpleSelector& part : selector.parts)
  {
    if (!part.id.empty())
    {
      ++specificity.ids;
    }
    specificity.classes += static_cast<int>(part.classes.size());
    if (!part.type.empty())
    {
      ++specificity.types;
    }
  }
  return specificity;
}

StyleSheet CssParser::ParseStyleSheet(std::string_view input) const
{
  const std::string css = StripComments(input);
  StyleSheet sheet;
  std::size_t position = 0;

  while (position < css.size())
  {
    const std::size_t open = css.find('{', position);
    if (open == std::string::npos)
    {
      break;
    }

    const std::size_t close = css.find('}', open + 1);
    if (close == std::string::npos)
    {
      break;
    }

    const std::string selector_text = Trim(std::string_view(css).substr(position, open - position));
    const std::string declaration_text =
        std::string(std::string_view(css).substr(open + 1, close - open - 1));
    const std::vector<Declaration> declarations = ParseDeclarationList(declaration_text);

    if (!selector_text.empty() && !declarations.empty())
    {
      for (const std::string& selector_part : SplitTopLevel(selector_text, ','))
      {
        Selector selector = ParseSelector(selector_part);
        if (selector.parts.empty())
        {
          continue;
        }

        sheet.rules.push_back({
            .selector = std::move(selector),
            .declarations = declarations,
            .source_order = sheet.rules.size(),
        });
      }
    }

    position = close + 1;
  }

  return sheet;
}

std::vector<Declaration> CssParser::ParseDeclarationList(std::string_view input) const
{
  std::vector<Declaration> declarations;
  const std::string css = StripComments(input);

  std::size_t begin = 0;
  for (std::size_t position = 0; position <= css.size(); ++position)
  {
    if (position != css.size() && css[position] != ';')
    {
      continue;
    }

    const std::string raw_declaration = Trim(std::string_view(css).substr(begin, position - begin));
    begin = position + 1;
    if (raw_declaration.empty())
    {
      continue;
    }

    const std::size_t colon = raw_declaration.find(':');
    if (colon == std::string::npos)
    {
      continue;
    }

    std::string property = ToAsciiLower(Trim(std::string_view(raw_declaration).substr(0, colon)));
    std::string value = Trim(std::string_view(raw_declaration).substr(colon + 1));
    if (property.empty() || value.empty())
    {
      continue;
    }

    AppendExpandedDeclaration(declarations, std::move(property), std::move(value));
  }

  return declarations;
}

} // namespace speed::engine::css
