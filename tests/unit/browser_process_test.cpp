#include "browser/browser_process.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{

class FakeNetworkClient final : public speed::browser::NavigationNetworkClient
{
public:
  speed::ipc::navigation::NavigateResponse
  SendNavigateRequest(const speed::ipc::navigation::NavigateRequest& request) override
  {
    requests.push_back(request);
    return {
        .request_id = request.request_id,
        .status = speed::ipc::navigation::NavigateStatus::kAllowed,
        .aegis_reason = "unit test allow",
        .error_message = {},
        .document_body = "<html><body><p>" + request.url + "</p></body></html>",
    };
  }

  std::vector<speed::ipc::navigation::NavigateRequest> requests;
};

class FakeRendererClient final : public speed::browser::NavigationRendererClient
{
public:
  speed::base::Status
  SendCommitDocument(const speed::ipc::navigation::CommitDocument& commit) override
  {
    commits.push_back(commit);
    return speed::base::Status::Ok();
  }

  speed::base::Status
  SendCommitErrorPage(const speed::ipc::navigation::CommitErrorPage& commit) override
  {
    error_commits.push_back(commit);
    return speed::base::Status::Ok();
  }

  std::vector<speed::ipc::navigation::CommitDocument> commits;
  std::vector<speed::ipc::navigation::CommitErrorPage> error_commits;
};

} // namespace

int main()
{
  FakeNetworkClient network;
  FakeRendererClient renderer;
  speed::browser::BrowserProcess process(network, renderer);
  assert(process.state() == speed::browser::BrowserProcess::State::kCreated);
  assert(!process.running());
  assert(process.tabs().tab_count() == 0);

  speed::base::Status status = process.NavigateActiveTab("example.test");
  assert(!status.ok());

  status = process.Start();
  assert(status.ok());
  assert(process.running());
  assert(process.tabs().tab_count() == 1);
  assert(process.tabs().active_tab());
  assert(process.history().entry_count() == 1);
  assert(process.history().entries().front() == "about:blank");
  assert(network.requests.size() == 1);
  assert(network.requests.front().is_top_level);
  assert(renderer.commits.size() == 1);
  assert(renderer.error_commits.empty());
  assert(renderer.commits.front().url == "about:blank");
  const speed::browser::TabState* initial_tab =
      process.tabs().GetTabState(process.tabs().active_tab());
  assert(initial_tab != nullptr);
  assert(initial_tab->navigation_state == speed::browser::TabNavigationState::kCommitted);
  assert(initial_tab->current_url == "about:blank");

  status = process.Start();
  assert(!status.ok());

  status = process.NavigateActiveTab("Example.TEST/page");
  assert(status.ok());
  assert(process.history().entry_count() == 2);
  assert(process.history().entries().back() == "https://example.test/page");
  assert(network.requests.size() == 2);
  assert(network.requests.back().url == "https://example.test/page");
  assert(renderer.commits.size() == 2);
  assert(renderer.error_commits.empty());
  assert(renderer.commits.back().url == "https://example.test/page");
  assert(initial_tab->navigation_state == speed::browser::TabNavigationState::kCommitted);
  assert(initial_tab->current_url == "https://example.test/page");

  status = process.NavigateTab(speed::base::TabId::FromRaw(99), "example.test");
  assert(!status.ok());

  status = process.NavigateActiveTab("file:///tmp/speed");
  assert(!status.ok());
  assert(process.history().entry_count() == 2);
  assert(network.requests.size() == 2);
  assert(renderer.commits.size() == 2);
  assert(renderer.error_commits.empty());
  assert(initial_tab->navigation_state == speed::browser::TabNavigationState::kCommitted);

  status = process.HandleRendererCrash(process.tabs().active_tab(), "simulated crash");
  assert(status.ok());
  assert(initial_tab->navigation_state == speed::browser::TabNavigationState::kCrashed);
  assert(initial_tab->last_error == "simulated crash");

  status = process.Shutdown();
  assert(status.ok());
  assert(!process.running());
  assert(process.state() == speed::browser::BrowserProcess::State::kStopped);
  assert(process.tabs().tab_count() == 0);

  status = process.Shutdown();
  assert(!status.ok());

  return 0;
}
