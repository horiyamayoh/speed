#include "engine/html/html_parser.h"
#include "engine/layout/layout_engine.h"
#include "engine/style/style_resolver.h"

#include <cassert>
#include <string_view>

namespace
{

const speed::engine::layout::LayoutBox* FindBoxByName(const speed::engine::layout::LayoutBox& box,
                                                      std::string_view name)
{
  if (box.node_name == name)
  {
    return &box;
  }

  for (const speed::engine::layout::LayoutBox& child : box.children)
  {
    if (const speed::engine::layout::LayoutBox* found = FindBoxByName(child, name);
        found != nullptr)
    {
      return found;
    }
  }

  return nullptr;
}

bool ContainsBoxNamed(const speed::engine::layout::LayoutBox& box, std::string_view name)
{
  return FindBoxByName(box, name) != nullptr;
}

const speed::engine::layout::LayoutBox* FirstTextBox(const speed::engine::layout::LayoutBox& box)
{
  if (box.type == speed::engine::layout::LayoutBoxType::kText)
  {
    return &box;
  }

  for (const speed::engine::layout::LayoutBox& child : box.children)
  {
    if (const speed::engine::layout::LayoutBox* found = FirstTextBox(child); found != nullptr)
    {
      return found;
    }
  }

  return nullptr;
}

bool HasTextOnLaterLine(const speed::engine::layout::LayoutBox& box, int first_line_y)
{
  if (box.type == speed::engine::layout::LayoutBoxType::kText && box.rect.y > first_line_y)
  {
    return true;
  }

  for (const speed::engine::layout::LayoutBox& child : box.children)
  {
    if (HasTextOnLaterLine(child, first_line_y))
    {
      return true;
    }
  }

  return false;
}

void LaysOutBlockBoxWithEdgesAndInlineText()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document = parser.ParseFragment(
      "<html><head><style>"
      "body { width: 200px; }"
      "#box { margin: 10px; padding: 5px; border: 2px solid black; width: 100px; }"
      "</style></head><body>"
      "<div id=box>Hello Speed Browser Engine Text</div>"
      "</body></html>");

  const speed::engine::style::StyleResolver resolver;
  const speed::engine::style::StyledNode styled_document = resolver.Resolve(document);

  const speed::engine::layout::LayoutEngine layout_engine;
  const speed::engine::layout::LayoutResult layout =
      layout_engine.Layout(styled_document, {.width = 240, .height = 160});

  assert(layout.root.rect.width == 240);
  assert(layout.root.rect.height == 160);

  const speed::engine::layout::LayoutBox* box = FindBoxByName(layout.root, "div");
  assert(box != nullptr);
  assert(box->rect.x == 10);
  assert(box->rect.y == 10);
  assert(box->rect.width == 114);
  assert(box->rect.height > 14);

  const speed::engine::layout::LayoutBox* text = FirstTextBox(*box);
  assert(text != nullptr);
  assert(text->rect.x == 17);
  assert(text->rect.y == 17);
  assert(HasTextOnLaterLine(*box, text->rect.y));
}

void SkipsDisplayNoneHeadAndStyleBoxes()
{
  const speed::engine::html::HtmlParser parser;
  const speed::engine::dom::Node document =
      parser.ParseFragment("<html><head><style>.gone { display: none; }</style></head>"
                           "<body><p class=gone>Hidden</p><p>Visible</p></body></html>");

  const speed::engine::style::StyleResolver resolver;
  const speed::engine::style::StyledNode styled_document = resolver.Resolve(document);

  const speed::engine::layout::LayoutEngine layout_engine;
  const speed::engine::layout::LayoutResult layout =
      layout_engine.Layout(styled_document, {.width = 200, .height = 100});

  assert(!ContainsBoxNamed(layout.root, "head"));
  assert(!ContainsBoxNamed(layout.root, "style"));

  const speed::engine::layout::LayoutBox* paragraph = FindBoxByName(layout.root, "p");
  assert(paragraph != nullptr);
  const speed::engine::layout::LayoutBox* text = FirstTextBox(*paragraph);
  assert(text != nullptr);
  assert(text->text == "Visible");
}

} // namespace

int main()
{
  LaysOutBlockBoxWithEdgesAndInlineText();
  SkipsDisplayNoneHeadAndStyleBoxes();

  return 0;
}
