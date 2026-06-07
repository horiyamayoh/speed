#pragma once

#include "ipc/runtime/navigation_messages.h"
#include "network/fetch/network_service.h"

namespace speed::network
{

class NetworkProcess final
{
public:
  explicit NetworkProcess(NetworkService service = NetworkService{});

  [[nodiscard]] ipc::navigation::NavigateResponse
  HandleNavigateRequest(const ipc::navigation::NavigateRequest& request) const;

private:
  NetworkService service_;
};

int RunNetworkProcess(int ipc_fd = -1);

} // namespace speed::network
