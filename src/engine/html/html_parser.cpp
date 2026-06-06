#include "engine/html/html_parser.h"

#include <string>
#include <utility>

namespace speed::engine::html
{

dom::Node HtmlParser::ParseFragment(std::string_view input) const
{
  dom::Node document;
  document.type = dom::NodeType::kDocument;
  document.name = "#document";

  if (!input.empty())
  {
    dom::Node text_node;
    text_node.type = dom::NodeType::kText;
    text_node.text = std::string(input);
    document.children.push_back(std::move(text_node));
  }

  return document;
}

} // namespace speed::engine::html
