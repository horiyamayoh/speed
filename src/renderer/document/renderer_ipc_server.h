#pragma once

#include "base/result/status.h"
#include "ipc/runtime/framed_transport.h"
#include "renderer/document/renderer_process.h"

namespace speed::renderer
{

class RendererIpcServer final
{
public:
  RendererIpcServer(RendererProcess& process, ipc::FileDescriptorTransport transport);

  [[nodiscard]] base::Status RunOnce();
  [[nodiscard]] base::Status RunUntilClosed();

private:
  RendererProcess& process_;
  ipc::FileDescriptorTransport transport_;
};

} // namespace speed::renderer
