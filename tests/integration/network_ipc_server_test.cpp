#include "aegis/classifier/request_classifier.h"
#include "ipc/runtime/framed_transport.h"
#include "ipc/runtime/navigation_codec.h"
#include "network/fetch/fetch_adapter.h"
#include "network/network_ipc_server.h"
#include "network/network_process.h"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

namespace
{

class CountingFetchAdapter final : public speed::network::FetchAdapter
{
public:
  speed::network::FetchResult Fetch(const speed::network::FetchRequest& request) const override
  {
    ++fetch_count;
    return speed::network::FetchResult::Success("<html><body>" + request.url + "</body></html>");
  }

  mutable int fetch_count{0};
};

[[nodiscard]] speed::ipc::navigation::NavigateResponse
SendRequestThroughServer(speed::network::NetworkIpcServer& server,
                         speed::ipc::FileDescriptorTransport& client,
                         const speed::ipc::navigation::NavigateRequest& request)
{
  speed::base::Status status =
      client.SendMessage(speed::ipc::navigation::EncodeNavigateRequest(request));
  assert(status.ok());

  status = server.RunOnce();
  assert(status.ok());

  speed::ipc::Message message({}, {});
  status = client.ReceiveMessage(message);
  assert(status.ok());

  speed::ipc::navigation::NavigateResponse response;
  status = speed::ipc::navigation::DecodeNavigateResponse(message, response);
  assert(status.ok());
  return response;
}

} // namespace

int main()
{
  speed::aegis::RequestClassifier classifier;
  assert(classifier
             .AddBlockingRule({
                 .pattern = "ads.example",
                 .reason = "blocked before dispatch",
             })
             .ok());

  auto fetch_adapter = std::make_unique<CountingFetchAdapter>();
  const CountingFetchAdapter* const fetch_counter = fetch_adapter.get();
  std::unique_ptr<speed::network::FetchAdapter> owned_fetch_adapter = std::move(fetch_adapter);
  speed::network::NetworkProcess process(
      speed::network::NetworkService(std::move(classifier), std::move(owned_fetch_adapter)));

  speed::ipc::LocalTransportPair pair;
  speed::base::Status status = speed::ipc::CreateLocalTransportPair(pair);
  assert(status.ok());

  speed::network::NetworkIpcServer server(process, std::move(pair.second));

  speed::ipc::navigation::NavigateResponse response =
      SendRequestThroughServer(server,
                               pair.first,
                               {
                                   .request_id = speed::base::RequestId::FromRaw(1),
                                   .tab_id = speed::base::TabId::FromRaw(2),
                                   .url = "https://ads.example/tracker",
                                   .is_top_level = true,
                               });
  assert(response.status == speed::ipc::navigation::NavigateStatus::kBlocked);
  assert(response.aegis_reason == "blocked before dispatch");
  assert(fetch_counter->fetch_count == 0);

  response = SendRequestThroughServer(server,
                                      pair.first,
                                      {
                                          .request_id = speed::base::RequestId::FromRaw(2),
                                          .tab_id = speed::base::TabId::FromRaw(2),
                                          .url = "https://allowed.example/page",
                                          .is_top_level = true,
                                      });
  assert(response.status == speed::ipc::navigation::NavigateStatus::kAllowed);
  assert(response.document_body.find("https://allowed.example/page") != std::string::npos);
  assert(fetch_counter->fetch_count == 1);

  return 0;
}
