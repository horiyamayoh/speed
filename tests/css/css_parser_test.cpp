#include "engine/css/css_parser.h"

#include <cassert>
#include <string_view>

namespace
{

const speed::engine::css::Declaration&
FindDeclaration(const std::vector<speed::engine::css::Declaration>& declarations,
                std::string_view property)
{
  for (const speed::engine::css::Declaration& declaration : declarations)
  {
    if (declaration.property == property)
    {
      return declaration;
    }
  }

  assert(false);
  return declarations.front();
}

void ParsesMvpSelectors()
{
  const speed::engine::css::CssParser parser;
  const speed::engine::css::StyleSheet sheet =
      parser.ParseStyleSheet("div.card #title { display: block; color: red; }"
                             "p.note, span.callout { font-size: 18px; }");

  assert(sheet.rules.size() == 3);

  const speed::engine::css::Selector& descendant = sheet.rules[0].selector;
  assert(descendant.parts.size() == 2);
  assert(descendant.parts[0].type == "div");
  assert(descendant.parts[0].classes.size() == 1);
  assert(descendant.parts[0].classes[0] == "card");
  assert(descendant.parts[1].id == "title");

  const speed::engine::css::Specificity specificity =
      speed::engine::css::CalculateSpecificity(descendant);
  assert(specificity.ids == 1);
  assert(specificity.classes == 1);
  assert(specificity.types == 1);

  assert(sheet.rules[1].selector.parts[0].type == "p");
  assert(sheet.rules[1].selector.parts[0].classes[0] == "note");
  assert(sheet.rules[2].selector.parts[0].type == "span");
  assert(sheet.rules[2].selector.parts[0].classes[0] == "callout");
}

void ExpandsBoxAndBorderShorthands()
{
  const speed::engine::css::CssParser parser;
  const std::vector<speed::engine::css::Declaration> declarations =
      parser.ParseDeclarationList("margin: 1px 2px 3px 4px; padding: 5px 6px;"
                                  "border: 2px solid #123456; width: 80px;");

  assert(FindDeclaration(declarations, "margin-top").value == "1px");
  assert(FindDeclaration(declarations, "margin-right").value == "2px");
  assert(FindDeclaration(declarations, "margin-bottom").value == "3px");
  assert(FindDeclaration(declarations, "margin-left").value == "4px");

  assert(FindDeclaration(declarations, "padding-top").value == "5px");
  assert(FindDeclaration(declarations, "padding-right").value == "6px");
  assert(FindDeclaration(declarations, "padding-bottom").value == "5px");
  assert(FindDeclaration(declarations, "padding-left").value == "6px");

  assert(FindDeclaration(declarations, "border-top-width").value == "2px");
  assert(FindDeclaration(declarations, "border-right-width").value == "2px");
  assert(FindDeclaration(declarations, "border-bottom-width").value == "2px");
  assert(FindDeclaration(declarations, "border-left-width").value == "2px");
  assert(FindDeclaration(declarations, "border-color").value == "#123456");
  assert(FindDeclaration(declarations, "width").value == "80px");
}

} // namespace

int main()
{
  ParsesMvpSelectors();
  ExpandsBoxAndBorderShorthands();

  return 0;
}
