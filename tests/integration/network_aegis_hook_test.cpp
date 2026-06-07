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

  const speed::ipc::navigation::NavigateResponse response = service.FetchNavigation({
      .request_id = speed::base::RequestId::FromRaw(2),
      .tab_id = speed::base::TabId::FromRaw(7),
      .url = "https://ads.example/banner.png",
      .is_top_level = true,
  });
  assert(response.request_id.value() == 2);
  assert(response.status == speed::ipc::navigation::NavigateStatus::kBlocked);
  assert(response.aegis_reason == "test ad server");
  assert(response.body_ref.empty());

  return 0;
}
