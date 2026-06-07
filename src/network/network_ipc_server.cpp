#include "network/network_ipc_server.h"

#include "ipc/runtime/navigation_codec.h"

#include <string>
#include <utility>

namespace speed::network
{

namespace
{

[[nodiscard]] bool IsClosedTransportStatus(const base::Status& status)
{
  return status.message().find("IPC transport closed") != std::string::npos;
}

} // namespace

NetworkIpcServer::NetworkIpcServer(NetworkProcess& process, ipc::FileDescriptorTransport transport)
    : process_(process),
      transport_(std::move(transport))
{}

base::Status NetworkIpcServer::RunOnce()
{
  ipc::Message message({}, {});
  base::Status status = transport_.ReceiveMessage(message);
  if (!status.ok())
  {
    return status;
  }

  ipc::navigation::NavigateRequest request;
  status = ipc::navigation::DecodeNavigateRequest(message, request);
  if (!status.ok())
  {
    return status;
  }

  const ipc::navigation::NavigateResponse response = process_.HandleNavigateRequest(request);
  return transport_.SendMessage(ipc::navigation::EncodeNavigateResponse(response));
}

base::Status NetworkIpcServer::RunUntilClosed()
{
  for (;;)
  {
    const base::Status status = RunOnce();
    if (!status.ok())
    {
      return IsClosedTransportStatus(status) ? base::Status::Ok() : status;
    }
  }
}

} // namespace speed::network
