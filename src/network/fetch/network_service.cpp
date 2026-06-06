#include "network/fetch/network_service.h"

#include <utility>

namespace speed::network
{

NetworkService::NetworkService(aegis::RequestClassifier classifier)
    : classifier_(std::move(classifier))
{}

NetworkResult NetworkService::PrepareRequest(const NetworkRequest& request) const
{
  const aegis::Classification classification = classifier_.ClassifyUrl(request.url);
  return {
      .would_dispatch = classification.kind == aegis::DecisionKind::kAllow,
      .aegis_decision = classification,
  };
}

} // namespace speed::network
