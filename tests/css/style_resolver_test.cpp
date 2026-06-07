#include "engine/html/html_parser.h"
#include "engine/style/style_resolver.h"

#include <cassert>
#include <string_view>

namespace
{

const speed::engine::style::StyledNode*
FindElementByName(const speed::engine::style::StyledNode& node, std::string_view name)
{
  if (node.node != nullptr && node.node->name == name)
  {
    return &node;
  }

  for (const speed::engine::style::StyledNode& child : node.children)
  {
    if (const speed::engine::style::StyledNode* found = FindElementByName(child, name);
        found != nullptr)
    {
      return found;
    }
  }

  return nullptr;
}

const speed::engine::style::StyledNode*
FindElementById(const speed::engine::style::StyledNode& node, std::string_view id)
{
  if (node.node != nullptr)
  {
    for (const speed::engine::dom::Attribute& attribute : node.node->attributes)
    {
      if (attribute.name == "id" && attribute.value == id)
      {
        return &node;
      }
    }
  }

  for (const speed::engine::style::StyledNode& child : node.children)
  {
    if (const speed::engine::style::StyledNode* found = FindElementById(child, id);
        found != nullptr)
    {
      return found;
    }
  }

  return nullptr;
}

void ResolvesCascadeSpecificityInlineAndInheritance()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document =
      parser.ParseFragment("<html><head><style>"
                           "div { color: red; }"
                           ".card { color: blue; font-size: 20px; }"
                           "#hero span { color: #123456; }"
                           ".hidden { display: none; }"
                           "</style></head><body>"
                           "<div id=hero class=card style='color: green; padding: 1px 2px;'>"
                           "Hello <span id=label>World</span><p id=gone class=hidden>Hidden</p>"
                           "</div></body></html>");

  const speed::engine::style::StyleResolver resolver;
  const speed::engine::style::StyledNode styled_document = resolver.Resolve(document);

  const speed::engine::style::StyledNode* hero = FindElementById(styled_document, "hero");
  assert(hero != nullptr);
  const speed::engine::style::Color green{.red = 0, .green = 128, .blue = 0, .alpha = 255};
  assert(hero->style.color == green);
  assert(hero->style.font_size_px == 20);
  assert(hero->style.padding.top == 1);
  assert(hero->style.padding.right == 2);
  assert(hero->style.padding.bottom == 1);
  assert(hero->style.padding.left == 2);

  const speed::engine::style::StyledNode* label = FindElementById(styled_document, "label");
  assert(label != nullptr);
  const speed::engine::style::Color label_color{.red = 18, .green = 52, .blue = 86, .alpha = 255};
  assert(label->style.color == label_color);
  assert(label->style.font_size_px == 20);

  const speed::engine::style::StyledNode* gone = FindElementById(styled_document, "gone");
  assert(gone != nullptr);
  assert(gone->style.display == speed::engine::style::Display::kNone);
}

void SuppressesHeadAndStyleLayoutByDefault()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document = parser.ParseFragment(
      "<html><head><style>body { color: blue; }</style></head><body>Text</body></html>");

  const speed::engine::style::StyleResolver resolver;
  const speed::engine::style::StyledNode styled_document = resolver.Resolve(document);

  const speed::engine::style::StyledNode* head = FindElementByName(styled_document, "head");
  const speed::engine::style::StyledNode* style = FindElementByName(styled_document, "style");
  assert(head != nullptr);
  assert(style != nullptr);
  assert(head->style.display == speed::engine::style::Display::kNone);
  assert(style->style.display == speed::engine::style::Display::kNone);
}

} // namespace

int main()
{
  ResolvesCascadeSpecificityInlineAndInheritance();
  SuppressesHeadAndStyleLayoutByDefault();

  return 0;
}
