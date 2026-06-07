#include "aegis/classifier/request_classifier.h"
#include "network/fetch/fetch_adapter.h"
#include "network/fetch/network_service.h"

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

class RecordingFetchAdapter final : public speed::network::FetchAdapter
{
public:
  explicit RecordingFetchAdapter(std::string body)
      : body_(std::move(body))
  {}

  speed::network::FetchResult Fetch(const speed::network::FetchRequest& request) const override
  {
    requested_urls.push_back(request.url);
    return speed::network::FetchResult::Success(body_);
  }

  mutable std::vector<std::string> requested_urls;

private:
  std::string body_;
};

} // namespace

int main()
{
  speed::aegis::RequestClassifier classifier;
  (void)classifier.AddBlockingRule({.pattern = "ads.example", .reason = "test ad server"});

  constexpr char kAllowedBody[] = "<html><body>adapter document</body></html>";
  auto adapter = std::make_unique<RecordingFetchAdapter>(kAllowedBody);
  const RecordingFetchAdapter* adapter_view = adapter.get();

  const speed::network::NetworkService service(std::move(classifier), std::move(adapter));
  const speed::network::NetworkResult result = service.PrepareRequest({
      .request_id = speed::base::RequestId::FromRaw(1),
      .url = "https://ads.example/banner.png",
  });

  assert(!result.would_dispatch);
  assert(result.aegis_decision.kind == speed::aegis::DecisionKind::kBlock);

  const speed::ipc::navigation::NavigateResponse allowed_response = service.FetchNavigation({
      .request_id = speed::base::RequestId::FromRaw(2),
      .tab_id = speed::base::TabId::FromRaw(7),
      .url = "https://allowed.example/page",
      .is_top_level = true,
  });
  assert(allowed_response.request_id.value() == 2);
  assert(allowed_response.status == speed::ipc::navigation::NavigateStatus::kAllowed);
  assert(allowed_response.document_body == kAllowedBody);
  assert(allowed_response.error_message.empty());
  assert(adapter_view->requested_urls.size() == 1);
  assert(adapter_view->requested_urls.back() == "https://allowed.example/page");

  const speed::ipc::navigation::NavigateResponse blocked_response = service.FetchNavigation({
      .request_id = speed::base::RequestId::FromRaw(3),
      .tab_id = speed::base::TabId::FromRaw(7),
      .url = "https://ads.example/banner.png",
      .is_top_level = true,
  });
  assert(blocked_response.request_id.value() == 3);
  assert(blocked_response.status == speed::ipc::navigation::NavigateStatus::kBlocked);
  assert(blocked_response.aegis_reason == "test ad server");
  assert(blocked_response.document_body.empty());
  assert(adapter_view->requested_urls.size() == 1);

  const speed::network::NetworkService default_service;
  const speed::ipc::navigation::NavigateResponse about_blank_response =
      default_service.FetchNavigation({
          .request_id = speed::base::RequestId::FromRaw(4),
          .tab_id = speed::base::TabId::FromRaw(7),
          .url = "about:blank",
          .is_top_level = true,
      });
  assert(about_blank_response.request_id.value() == 4);
  assert(about_blank_response.status == speed::ipc::navigation::NavigateStatus::kAllowed);
  assert(!about_blank_response.document_body.empty());
  assert(about_blank_response.error_message.empty());

  return 0;
}
