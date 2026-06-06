#pragma once

#include "aegis/rules/rule.h"
#include "base/result/status.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace speed::aegis
{

enum class DecisionKind : std::uint8_t
{
  kAllow,
  kBlock,
};

struct Classification final
{
  DecisionKind kind{DecisionKind::kAllow};
  std::string reason;
  std::string matched_pattern;

  static Classification Allow();
  static Classification Block(const Rule& rule);
};

class RequestClassifier final
{
public:
  [[nodiscard]] base::Status AddBlockingRule(Rule rule);
  [[nodiscard]] Classification ClassifyUrl(std::string_view url) const;

private:
  std::vector<Rule> rules_;
};

} // namespace speed::aegis
