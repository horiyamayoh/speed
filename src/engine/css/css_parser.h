#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace speed::engine::css
{

struct Declaration final
{
  std::string property;
  std::string value;
};

struct SimpleSelector final
{
  std::string type;
  std::string id;
  std::vector<std::string> classes;
};

struct Selector final
{
  std::vector<SimpleSelector> parts;
};

struct CssRule final
{
  Selector selector;
  std::vector<Declaration> declarations;
  std::size_t source_order{0};
};

struct StyleSheet final
{
  std::vector<CssRule> rules;
};

struct Specificity final
{
  int ids{0};
  int classes{0};
  int types{0};
};

[[nodiscard]] bool operator<(const Specificity& left, const Specificity& right);
[[nodiscard]] bool operator==(const Specificity& left, const Specificity& right);
[[nodiscard]] Specificity CalculateSpecificity(const Selector& selector);

class CssParser final
{
public:
  [[nodiscard]] StyleSheet ParseStyleSheet(std::string_view input) const;
  [[nodiscard]] std::vector<Declaration> ParseDeclarationList(std::string_view input) const;
};

} // namespace speed::engine::css
