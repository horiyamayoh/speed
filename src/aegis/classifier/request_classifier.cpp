#include "aegis/classifier/request_classifier.h"

#include <utility>

namespace speed::aegis
{

Classification Classification::Allow()
{
  return {
      .kind = DecisionKind::kAllow,
      .reason = "no matching Aegis rule",
      .matched_pattern = {},
  };
}

Classification Classification::Block(const Rule& rule)
{
  return {
      .kind = DecisionKind::kBlock,
      .reason = rule.reason.empty() ? "blocked by Aegis rule" : rule.reason,
      .matched_pattern = rule.pattern,
  };
}

base::Status RequestClassifier::AddBlockingRule(Rule rule)
{
  if (rule.pattern.empty())
  {
    return base::Status::Error("Aegis rule pattern must not be empty");
  }

  rules_.push_back(std::move(rule));
  return base::Status::Ok();
}

Classification RequestClassifier::ClassifyUrl(std::string_view url) const
{
  for (const Rule& rule : rules_)
  {
    if (url.find(std::string_view(rule.pattern)) != std::string_view::npos)
    {
      return Classification::Block(rule);
    }
  }

  return Classification::Allow();
}

} // namespace speed::aegis
