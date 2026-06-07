#include "browser/browser_process.h"
#include "base/logging/logging.h"
#include "network/network_process.h"
#include "renderer/document/renderer_process.h"

namespace
{

class InMemoryNetworkClient final : public speed::browser::NavigationNetworkClient
{
public:
  explicit InMemoryNetworkClient(speed::network::NetworkProcess& process)
      : process_(process)
  {}

  speed::ipc::navigation::NavigateResponse
  SendNavigateRequest(const speed::ipc::navigation::NavigateRequest& request) override
  {
    return process_.HandleNavigateRequest(request);
  }

private:
  speed::network::NetworkProcess& process_;
};

class InMemoryRendererClient final : public speed::browser::NavigationRendererClient
{
public:
  explicit InMemoryRendererClient(speed::renderer::RendererProcess& process)
      : process_(process)
  {}

  speed::base::Status
  SendCommitDocument(const speed::ipc::navigation::CommitDocument& commit) override
  {
    return process_.CommitDocument(commit);
  }

private:
  speed::renderer::RendererProcess& process_;
};

} // namespace

int main()
{
  speed::network::NetworkProcess network_process;
  speed::renderer::RendererProcess renderer_process;
  InMemoryNetworkClient network_client(network_process);
  InMemoryRendererClient renderer_client(renderer_process);

  speed::browser::BrowserProcess browser_process(network_client, renderer_client);
  const speed::base::Status start_status = browser_process.Start();
  if (!start_status.ok())
  {
    speed::base::Log(speed::base::LogLevel::kError, "browser", start_status.message());
    return 1;
  }

  return 0;
}
