#include "aegis/classifier/request_classifier.h"
#include "browser/browser_process.h"
#include "network/fetch/fetch_adapter.h"
#include "network/network_process.h"
#include "renderer/document/renderer_process.h"

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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
    requests.push_back(request);
    speed::ipc::navigation::NavigateResponse response = process_.HandleNavigateRequest(request);
    responses.push_back(response);
    return response;
  }

  std::vector<speed::ipc::navigation::NavigateRequest> requests;
  std::vector<speed::ipc::navigation::NavigateResponse> responses;

private:
  speed::network::NetworkProcess& process_;
};

class StaticFetchAdapter final : public speed::network::FetchAdapter
{
public:
  speed::network::FetchResult Fetch(const speed::network::FetchRequest& request) const override
  {
    return speed::network::FetchResult::Success(
        "<html><head><style>p { color: red; }</style></head><body><p>" + request.url +
        "</p></body></html>");
  }
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
    commits.push_back(commit);
    return process_.CommitDocument(commit);
  }

  speed::base::Status
  SendCommitErrorPage(const speed::ipc::navigation::CommitErrorPage& commit) override
  {
    error_commits.push_back(commit);
    return process_.CommitErrorPage(commit);
  }

  std::vector<speed::ipc::navigation::CommitDocument> commits;
  std::vector<speed::ipc::navigation::CommitErrorPage> error_commits;

private:
  speed::renderer::RendererProcess& process_;
};

} // namespace

int main()
{
  speed::aegis::RequestClassifier classifier;
  assert(classifier
             .AddBlockingRule({
                 .pattern = "ads.example",
                 .reason = "blocked navigation test",
             })
             .ok());

  speed::network::NetworkProcess network_process(speed::network::NetworkService(
      std::move(classifier), std::make_unique<StaticFetchAdapter>()));
  speed::renderer::RendererProcess renderer_process;
  InMemoryNetworkClient network(network_process);
  InMemoryRendererClient renderer(renderer_process);
  speed::browser::BrowserProcess browser(network, renderer);

  speed::base::Status status = browser.Start();
  assert(status.ok());
  const speed::base::TabId first_tab = browser.tabs().active_tab();
  assert(first_tab);
  assert(network.requests.size() == 1);
  assert(network.responses.back().status == speed::ipc::navigation::NavigateStatus::kAllowed);
  assert(renderer.commits.size() == 1);
  assert(renderer.error_commits.empty());
  assert(browser.history().entry_count() == 1);

  status = browser.NavigateTab(first_tab, "Allowed.EXAMPLE/page");
  assert(status.ok());
  assert(network.requests.size() == 2);
  assert(network.requests.back().url == "https://allowed.example/page");
  assert(network.requests.back().is_top_level);
  assert(network.responses.back().status == speed::ipc::navigation::NavigateStatus::kAllowed);
  assert(network.responses.back().aegis_reason == "no matching Aegis rule");
  assert(renderer.commits.size() == 2);
  assert(renderer.error_commits.empty());
  assert(renderer.commits.back().url == "https://allowed.example/page");
  assert(renderer_process.committed_document_count() == 2);
  std::optional<speed::renderer::CommittedDocument> rendered_document =
      renderer_process.LastCommittedDocument();
  assert(rendered_document.has_value());
  assert(!rendered_document->is_error_page);
  assert(rendered_document->document_body.find("https://allowed.example/page") !=
         std::string::npos);
  assert(!rendered_document->render_result.display_list.commands.empty());
  assert(browser.history().entry_count() == 2);
  assert(browser.history().entries().back() == "https://allowed.example/page");
  const speed::browser::TabState* first_state = browser.tabs().GetTabState(first_tab);
  assert(first_state != nullptr);
  assert(first_state->navigation_state == speed::browser::TabNavigationState::kCommitted);
  assert(first_state->current_url == "https://allowed.example/page");

  status = browser.NavigateTab(first_tab, "https://ads.example/tracker");
  assert(!status.ok());
  assert(network.requests.size() == 3);
  assert(network.responses.back().status == speed::ipc::navigation::NavigateStatus::kBlocked);
  assert(network.responses.back().aegis_reason == "blocked navigation test");
  assert(renderer.commits.size() == 2);
  assert(renderer.error_commits.size() == 1);
  assert(renderer.error_commits.back().reason == speed::ipc::navigation::ErrorPageReason::kBlocked);
  assert(renderer_process.committed_document_count() == 3);
  rendered_document = renderer_process.LastCommittedDocument();
  assert(rendered_document.has_value());
  assert(rendered_document->is_error_page);
  assert(rendered_document->error_reason == speed::ipc::navigation::ErrorPageReason::kBlocked);
  assert(!rendered_document->render_result.display_list.commands.empty());
  assert(browser.history().entry_count() == 2);
  first_state = browser.tabs().GetTabState(first_tab);
  assert(first_state != nullptr);
  assert(first_state->navigation_state == speed::browser::TabNavigationState::kBlocked);
  assert(first_state->current_url == "https://allowed.example/page");
  assert(first_state->last_error == "blocked navigation test");

  status = browser.NavigateTab(first_tab, "file:///tmp/speed");
  assert(!status.ok());
  assert(network.requests.size() == 3);
  assert(renderer.commits.size() == 2);
  assert(renderer.error_commits.size() == 1);
  assert(browser.history().entry_count() == 2);

  const speed::base::TabId second_tab = browser.CreateTab();
  assert(second_tab);
  status = browser.NavigateTab(second_tab, "Second.EXAMPLE/page");
  assert(status.ok());
  assert(network.requests.size() == 4);
  assert(renderer.commits.size() == 3);
  assert(renderer.error_commits.size() == 1);
  assert(browser.history().entry_count() == 3);

  first_state = browser.tabs().GetTabState(first_tab);
  const speed::browser::TabState* second_state = browser.tabs().GetTabState(second_tab);
  assert(first_state != nullptr);
  assert(second_state != nullptr);
  assert(first_state->navigation_state == speed::browser::TabNavigationState::kBlocked);
  assert(first_state->current_url == "https://allowed.example/page");
  assert(second_state->navigation_state == speed::browser::TabNavigationState::kCommitted);
  assert(second_state->current_url == "https://second.example/page");

  status = browser.HandleRendererCrash(second_tab, "simulated renderer crash");
  assert(status.ok());
  second_state = browser.tabs().GetTabState(second_tab);
  assert(second_state != nullptr);
  assert(second_state->navigation_state == speed::browser::TabNavigationState::kCrashed);
  assert(first_state->navigation_state == speed::browser::TabNavigationState::kBlocked);

  return 0;
}
