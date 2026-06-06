#include "aegis/classifier/request_classifier.h"
#include "network/fetch/network_service.h"

#include <cassert>
#include <utility>

int main()
{
  speed::aegis::RequestClassifier classifier;
  (void)classifier.AddBlockingRule({.pattern = "ads.example", .reason = "test ad server"});

  const speed::network::NetworkService service(std::move(classifier));
  const speed::network::NetworkResult result = service.PrepareRequest({
      .request_id = speed::base::RequestId::FromRaw(1),
      .url = "https://ads.example/banner.png",
  });

  assert(!result.would_dispatch);
  assert(result.aegis_decision.kind == speed::aegis::DecisionKind::kBlock);

  return 0;
}
