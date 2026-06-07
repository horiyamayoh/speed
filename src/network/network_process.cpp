#include "network/network_process.h"

#include "base/logging/logging.h"

#include <utility>

namespace speed::network
{

NetworkProcess::NetworkProcess(NetworkService service)
    : service_(std::move(service))
{}

ipc::navigation::NavigateResponse
NetworkProcess::HandleNavigateRequest(const ipc::navigation::NavigateRequest& request) const
{
  return service_.FetchNavigation(request);
}

int RunNetworkProcess()
{
  base::Log(base::LogLevel::kInfo, "network", "network process stub ready");
  return 0;
}

} // namespace speed::network
