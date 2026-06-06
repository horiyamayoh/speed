#include "aegis/classifier/request_classifier.h"

#include <cassert>

int main()
{
  speed::aegis::RequestClassifier classifier;
  const speed::base::Status status =
      classifier.AddBlockingRule({.pattern = "tracker.example", .reason = "test tracker"});

  assert(status.ok());

  const speed::aegis::Classification blocked =
      classifier.ClassifyUrl("https://tracker.example/pixel.gif");
  assert(blocked.kind == speed::aegis::DecisionKind::kBlock);
  assert(blocked.matched_pattern == "tracker.example");

  const speed::aegis::Classification allowed =
      classifier.ClassifyUrl("https://example.test/index.html");
  assert(allowed.kind == speed::aegis::DecisionKind::kAllow);

  return 0;
}
