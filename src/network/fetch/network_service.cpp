#include "network/fetch/network_service.h"

#include <string>
#include <string_view>
#include <utility>

namespace
{

[[nodiscard]] std::string StubBodyRefForUrl(std::string_view url)
{
  return "stub-document:" + std::string(url);
}

} // namespace

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

ipc::navigation::NavigateResponse
NetworkService::FetchNavigation(const ipc::navigation::NavigateRequest& request) const
{
  if (!ipc::navigation::IsValidNavigateRequest(request))
  {
    return {
        .request_id = request.request_id,
        .status = ipc::navigation::NavigateStatus::kFailed,
        .aegis_reason = {},
        .error_message = "invalid navigation request",
        .body_ref = {},
    };
  }

  const NetworkResult prepared = PrepareRequest({
      .request_id = request.request_id,
      .url = request.url,
  });

  if (!prepared.would_dispatch)
  {
    return {
        .request_id = request.request_id,
        .status = ipc::navigation::NavigateStatus::kBlocked,
        .aegis_reason = prepared.aegis_decision.reason,
        .error_message = {},
        .body_ref = {},
    };
  }

  return {
      .request_id = request.request_id,
      .status = ipc::navigation::NavigateStatus::kAllowed,
      .aegis_reason = prepared.aegis_decision.reason,
      .error_message = {},
      .body_ref = StubBodyRefForUrl(request.url),
  };
}

} // namespace speed::network
