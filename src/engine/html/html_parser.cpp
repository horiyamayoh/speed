#include "engine/html/html_parser.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

enum class TokenType
{
  kText,
  kStartTag,
  kEndTag,
};

struct Token final
{
  TokenType type{TokenType::kText};
  std::string name;
  std::string text;
  std::vector<speed::engine::dom::Attribute> attributes;
  bool self_closing{false};
};

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

[[nodiscard]] bool IsNameStartCharacter(char character)
{
  return IsAsciiAlpha(character);
}

[[nodiscard]] bool IsNameCharacter(char character)
{
  return IsAsciiAlpha(character) || IsAsciiDigit(character) || character == '-' ||
         character == '_' || character == ':';
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
  std::string result(input);
  for (char& character : result)
  {
    character = ToAsciiLower(character);
  }
  return result;
}

void SkipAsciiWhitespace(std::string_view input, std::size_t& position)
{
  while (position < input.size() && IsAsciiWhitespace(input[position]))
  {
    ++position;
  }
}

[[nodiscard]] bool
StartsWithAt(std::string_view input, std::size_t position, std::string_view prefix)
{
  return position <= input.size() && input.substr(position).starts_with(prefix);
}

void SkipUntilTagClose(std::string_view input, std::size_t& position)
{
  const std::size_t close = input.find('>', position);
  position = close == std::string_view::npos ? input.size() : close + 1;
}

void SkipComment(std::string_view input, std::size_t& position)
{
  const std::size_t close = input.find("-->", position + 4);
  position = close == std::string_view::npos ? input.size() : close + 3;
}

[[nodiscard]] std::string ParseName(std::string_view input, std::size_t& position)
{
  if (position >= input.size() || !IsNameStartCharacter(input[position]))
  {
    return {};
  }

  const std::size_t begin = position;
  ++position;
  while (position < input.size() && IsNameCharacter(input[position]))
  {
    ++position;
  }

  return ToAsciiLower(input.substr(begin, position - begin));
}

[[nodiscard]] std::string ParseAttributeValue(std::string_view input, std::size_t& position)
{
  if (position >= input.size())
  {
    return {};
  }

  const char quote = input[position];
  if (quote == '"' || quote == '\'')
  {
    ++position;
    const std::size_t begin = position;
    const std::size_t close = input.find(quote, position);
    if (close == std::string_view::npos)
    {
      position = input.size();
      return std::string(input.substr(begin));
    }

    position = close + 1;
    return std::string(input.substr(begin, close - begin));
  }

  const std::size_t begin = position;
  while (position < input.size() && !IsAsciiWhitespace(input[position]) && input[position] != '>')
  {
    if (input[position] == '/' && position + 1 < input.size() && input[position + 1] == '>')
    {
      break;
    }
    ++position;
  }

  return std::string(input.substr(begin, position - begin));
}

[[nodiscard]] speed::engine::dom::Attribute ParseAttribute(std::string_view input,
                                                           std::size_t& position)
{
  speed::engine::dom::Attribute attribute;
  attribute.name = ParseName(input, position);
  if (attribute.name.empty())
  {
    ++position;
    return attribute;
  }

  SkipAsciiWhitespace(input, position);
  if (position >= input.size() || input[position] != '=')
  {
    return attribute;
  }

  ++position;
  SkipAsciiWhitespace(input, position);
  attribute.value = ParseAttributeValue(input, position);
  return attribute;
}

[[nodiscard]] Token ParseEndTag(std::string_view input, std::size_t& position)
{
  position += 2;
  SkipAsciiWhitespace(input, position);
  Token token;
  token.type = TokenType::kEndTag;
  token.name = ParseName(input, position);
  SkipUntilTagClose(input, position);
  return token;
}

[[nodiscard]] Token ParseStartTag(std::string_view input, std::size_t& position)
{
  ++position;
  Token token;
  token.type = TokenType::kStartTag;
  token.name = ParseName(input, position);
  if (token.name.empty())
  {
    token.type = TokenType::kText;
    token.text = "<";
    return token;
  }

  while (position < input.size())
  {
    SkipAsciiWhitespace(input, position);
    if (position >= input.size())
    {
      break;
    }

    if (input[position] == '>')
    {
      ++position;
      break;
    }

    if (input[position] == '/')
    {
      token.self_closing = true;
      ++position;
      SkipAsciiWhitespace(input, position);
      if (position < input.size() && input[position] == '>')
      {
        ++position;
      }
      break;
    }

    speed::engine::dom::Attribute attribute = ParseAttribute(input, position);
    if (!attribute.name.empty())
    {
      token.attributes.push_back(std::move(attribute));
    }
  }

  return token;
}

[[nodiscard]] Token ParseText(std::string_view input, std::size_t& position)
{
  const std::size_t begin = position;
  const std::size_t next_tag = input.find('<', position);
  position = next_tag == std::string_view::npos ? input.size() : next_tag;

  return {
      .type = TokenType::kText,
      .name = {},
      .text = std::string(input.substr(begin, position - begin)),
      .attributes = {},
      .self_closing = false,
  };
}

[[nodiscard]] Token NextToken(std::string_view input, std::size_t& position)
{
  if (position >= input.size())
  {
    return {};
  }

  if (input[position] != '<')
  {
    return ParseText(input, position);
  }

  if (StartsWithAt(input, position, "<!--"))
  {
    SkipComment(input, position);
    return NextToken(input, position);
  }

  if (StartsWithAt(input, position, "<!") || StartsWithAt(input, position, "<?"))
  {
    SkipUntilTagClose(input, position);
    return NextToken(input, position);
  }

  if (StartsWithAt(input, position, "</"))
  {
    return ParseEndTag(input, position);
  }

  if (position + 1 < input.size() && IsNameStartCharacter(input[position + 1]))
  {
    return ParseStartTag(input, position);
  }

  ++position;
  return {
      .type = TokenType::kText,
      .name = {},
      .text = "<",
      .attributes = {},
      .self_closing = false,
  };
}

[[nodiscard]] bool IsSupportedTag(std::string_view tag_name)
{
  return tag_name == "html" || tag_name == "head" || tag_name == "body" || tag_name == "style" ||
         tag_name == "div" || tag_name == "span" || tag_name == "p" || tag_name == "a" ||
         tag_name == "img" || tag_name == "h1" || tag_name == "h2" || tag_name == "h3" ||
         tag_name == "h4" || tag_name == "h5" || tag_name == "h6" || tag_name == "ul" ||
         tag_name == "ol" || tag_name == "li" || tag_name == "br";
}

[[nodiscard]] bool IsVoidTag(std::string_view tag_name)
{
  return tag_name == "br" || tag_name == "img";
}

[[nodiscard]] speed::engine::dom::Node& NodeAt(speed::engine::dom::Node& root,
                                               const std::vector<std::size_t>& path)
{
  speed::engine::dom::Node* node = &root;
  for (const std::size_t child_index : path)
  {
    node = &node->children[child_index];
  }
  return *node;
}

void AppendText(speed::engine::dom::Node& current, std::string text)
{
  if (text.empty())
  {
    return;
  }

  if (!current.children.empty() &&
      current.children.back().type == speed::engine::dom::NodeType::kText)
  {
    current.children.back().text += text;
    return;
  }

  speed::engine::dom::Node text_node;
  text_node.type = speed::engine::dom::NodeType::kText;
  text_node.text = std::move(text);
  current.children.push_back(std::move(text_node));
}

void HandleStartTag(speed::engine::dom::Node& document,
                    std::vector<std::size_t>& open_path,
                    Token token)
{
  if (!IsSupportedTag(token.name))
  {
    return;
  }

  speed::engine::dom::Node element;
  element.type = speed::engine::dom::NodeType::kElement;
  element.name = std::move(token.name);
  element.attributes = std::move(token.attributes);

  speed::engine::dom::Node& current = NodeAt(document, open_path);
  current.children.push_back(std::move(element));
  if (token.self_closing || IsVoidTag(current.children.back().name))
  {
    return;
  }

  open_path.push_back(current.children.size() - 1);
}

void HandleEndTag(speed::engine::dom::Node& document,
                  std::vector<std::size_t>& open_path,
                  const Token& token)
{
  if (!IsSupportedTag(token.name) || IsVoidTag(token.name))
  {
    return;
  }

  for (std::size_t depth = open_path.size(); depth > 0; --depth)
  {
    std::vector<std::size_t> candidate_path(open_path.begin(),
                                            open_path.begin() + static_cast<std::ptrdiff_t>(depth));
    if (NodeAt(document, candidate_path).name == token.name)
    {
      open_path.resize(depth - 1);
      return;
    }
  }
}

} // namespace

namespace speed::engine::html
{

dom::Node HtmlParser::ParseFragment(std::string_view input) const
{
  dom::Node document;
  document.type = dom::NodeType::kDocument;
  document.name = "#document";

  std::vector<std::size_t> open_path;
  std::size_t position = 0;
  while (position < input.size())
  {
    Token token = NextToken(input, position);
    switch (token.type)
    {
    case TokenType::kText:
      AppendText(NodeAt(document, open_path), std::move(token.text));
      break;
    case TokenType::kStartTag:
      HandleStartTag(document, open_path, std::move(token));
      break;
    case TokenType::kEndTag:
      HandleEndTag(document, open_path, token);
      break;
    }
  }

  return document;
}

} // namespace speed::engine::html
