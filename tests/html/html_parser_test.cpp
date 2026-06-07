#include "engine/html/html_parser.h"

#include <cassert>
#include <cstddef>
#include <string_view>

namespace
{

const speed::engine::dom::Node& Child(const speed::engine::dom::Node& node, std::size_t index)
{
  assert(index < node.children.size());
  return node.children[index];
}

void AssertText(const speed::engine::dom::Node& node, std::string_view text)
{
  assert(node.type == speed::engine::dom::NodeType::kText);
  assert(node.text == text);
  assert(node.name.empty());
  assert(node.children.empty());
}

void AssertElement(const speed::engine::dom::Node& node, std::string_view name)
{
  assert(node.type == speed::engine::dom::NodeType::kElement);
  assert(node.name == name);
  assert(node.text.empty());
}

void AssertAttribute(const speed::engine::dom::Attribute& attribute,
                     std::string_view name,
                     std::string_view value)
{
  assert(attribute.name == name);
  assert(attribute.value == value);
}

void PlainTextBecomesDocumentText()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document = parser.ParseFragment("plain text");

  assert(document.type == speed::engine::dom::NodeType::kDocument);
  assert(document.name == "#document");
  assert(document.children.size() == 1);
  AssertText(Child(document, 0), "plain text");
}

void BuildsNestedSupportedElements()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document =
      parser.ParseFragment("<div><p>Hello <span>Speed</span></p></div>");

  assert(document.children.size() == 1);
  const speed::engine::dom::Node& div = Child(document, 0);
  AssertElement(div, "div");
  assert(div.children.size() == 1);

  const speed::engine::dom::Node& paragraph = Child(div, 0);
  AssertElement(paragraph, "p");
  assert(paragraph.children.size() == 2);
  AssertText(Child(paragraph, 0), "Hello ");

  const speed::engine::dom::Node& span = Child(paragraph, 1);
  AssertElement(span, "span");
  assert(span.children.size() == 1);
  AssertText(Child(span, 0), "Speed");
}

void PreservesAttributesInOrder()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document = parser.ParseFragment(
      "<A ID=\"top\" class='primary link' href=https://example.test hidden>Speed</A>");

  assert(document.children.size() == 1);
  const speed::engine::dom::Node& anchor = Child(document, 0);
  AssertElement(anchor, "a");
  assert(anchor.attributes.size() == 4);
  AssertAttribute(anchor.attributes[0], "id", "top");
  AssertAttribute(anchor.attributes[1], "class", "primary link");
  AssertAttribute(anchor.attributes[2], "href", "https://example.test");
  AssertAttribute(anchor.attributes[3], "hidden", "");
  assert(anchor.children.size() == 1);
  AssertText(Child(anchor, 0), "Speed");
}

void HandlesVoidElementsWithoutClosingTags()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document =
      parser.ParseFragment("<div>Line<br><img src=x>After</div>");

  assert(document.children.size() == 1);
  const speed::engine::dom::Node& div = Child(document, 0);
  AssertElement(div, "div");
  assert(div.children.size() == 4);
  AssertText(Child(div, 0), "Line");
  AssertElement(Child(div, 1), "br");
  assert(Child(div, 1).children.empty());
  AssertElement(Child(div, 2), "img");
  assert(Child(div, 2).children.empty());
  assert(Child(div, 2).attributes.size() == 1);
  AssertAttribute(Child(div, 2).attributes[0], "src", "x");
  AssertText(Child(div, 3), "After");
}

void TransparentlyIgnoresUnsupportedTags()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document = parser.ParseFragment("<main><p>Text</p></main>");

  assert(document.children.size() == 1);
  const speed::engine::dom::Node& paragraph = Child(document, 0);
  AssertElement(paragraph, "p");
  assert(paragraph.children.size() == 1);
  AssertText(Child(paragraph, 0), "Text");
}

void HandlesMismatchedAndUnmatchedClosingTags()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document =
      parser.ParseFragment("</span><div><p>Text</div>Tail</p>");

  assert(document.children.size() == 2);
  const speed::engine::dom::Node& div = Child(document, 0);
  AssertElement(div, "div");
  assert(div.children.size() == 1);

  const speed::engine::dom::Node& paragraph = Child(div, 0);
  AssertElement(paragraph, "p");
  assert(paragraph.children.size() == 1);
  AssertText(Child(paragraph, 0), "Text");
  AssertText(Child(document, 1), "Tail");
}

void IgnoresCommentsAndDeclarations()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document = parser.ParseFragment(
      "<!doctype html><div>Hello<!-- hidden --><span>Speed</span><?ignored?></div>");

  assert(document.children.size() == 1);
  const speed::engine::dom::Node& div = Child(document, 0);
  AssertElement(div, "div");
  assert(div.children.size() == 2);
  AssertText(Child(div, 0), "Hello");

  const speed::engine::dom::Node& span = Child(div, 1);
  AssertElement(span, "span");
  assert(span.children.size() == 1);
  AssertText(Child(span, 0), "Speed");
}

} // namespace

int main()
{
  PlainTextBecomesDocumentText();
  BuildsNestedSupportedElements();
  PreservesAttributesInOrder();
  HandlesVoidElementsWithoutClosingTags();
  TransparentlyIgnoresUnsupportedTags();
  HandlesMismatchedAndUnmatchedClosingTags();
  IgnoresCommentsAndDeclarations();

  return 0;
}
