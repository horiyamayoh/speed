#pragma once

#include "base/result/status.h"
#include "ipc/runtime/framed_transport.h"
#include "network/network_process.h"

namespace speed::network
{

class NetworkIpcServer final
{
public:
  NetworkIpcServer(NetworkProcess& process, ipc::FileDescriptorTransport transport);

  [[nodiscard]] base::Status RunOnce();
  [[nodiscard]] base::Status RunUntilClosed();

private:
  NetworkProcess& process_;
  ipc::FileDescriptorTransport transport_;
};

} // namespace speed::network
