#include "network/network_process.h"

#include "base/logging/logging.h"
#include "network/network_ipc_server.h"

#include <utility>

namespace speed::network
{

namespace
{

[[nodiscard]] NetworkProcess BuildDefaultRuntimeNetworkProcess()
{
  aegis::RequestClassifier classifier;
  (void)classifier.AddBlockingRule({
      .pattern = "ads.example",
      .reason = "blocked by embedded Aegis MVP rule",
  });
  return NetworkProcess(NetworkService(std::move(classifier)));
}

} // namespace

NetworkProcess::NetworkProcess(NetworkService service)
    : service_(std::move(service))
{}

ipc::navigation::NavigateResponse
NetworkProcess::HandleNavigateRequest(const ipc::navigation::NavigateRequest& request) const
{
  return service_.FetchNavigation(request);
}

int RunNetworkProcess(int ipc_fd)
{
  if (ipc_fd >= 0)
  {
    NetworkProcess process = BuildDefaultRuntimeNetworkProcess();
    NetworkIpcServer server(process, ipc::FileDescriptorTransport(ipc_fd));
    const base::Status status = server.RunUntilClosed();
    if (!status.ok())
    {
      base::Log(base::LogLevel::kError, "network", status.message());
      return 1;
    }

    return 0;
  }

  base::Log(base::LogLevel::kInfo, "network", "network process stub ready");
  return 0;
}

} // namespace speed::network
