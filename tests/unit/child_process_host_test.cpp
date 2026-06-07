#include "browser/browser_process.h"
#include "browser/process_host/child_process_host.h"

#include <cassert>
#include <chrono>
#include <filesystem>
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
    return {
        .request_id = request.request_id,
        .status = speed::ipc::navigation::NavigateStatus::kAllowed,
        .aegis_reason = "unit test allow",
        .error_message = {},
        .document_body = "<html><body></body></html>",
    };
  }
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

[[nodiscard]] std::filesystem::path ShellPath()
{
  return "/bin/sh";
}

void RecordsCleanExit()
{
  if (!std::filesystem::exists(ShellPath()))
  {
    return;
  }

  speed::browser::ChildProcessHost host({
      .role = speed::browser::ChildProcessRole::kUtility,
      .executable_path = ShellPath(),
      .arguments = {"-c", "exit 0"},
      .tab_id = {},
  });

  speed::base::Status status = host.Launch();
  assert(status.ok());
  assert(host.state() == speed::browser::ChildProcessState::kRunning);

  status = host.WaitForExitFor(std::chrono::seconds(5));
  assert(status.ok());
  assert(host.state() == speed::browser::ChildProcessState::kExited);
  assert(host.exit_status().has_value());
  assert(host.exit_status()->exited);
  assert(host.exit_status()->exit_code == 0);
}

void ConvertsCrashToExplicitStateAndTabCrash()
{
  if (!std::filesystem::exists(ShellPath()))
  {
    return;
  }

  FakeNetworkClient network;
  FakeRendererClient renderer;
  speed::browser::BrowserProcess browser(network, renderer);
  speed::base::Status status = browser.Start();
  assert(status.ok());
  const speed::base::TabId tab_id = browser.tabs().active_tab();
  assert(tab_id);

  speed::browser::ChildProcessHost host({
      .role = speed::browser::ChildProcessRole::kRenderer,
      .executable_path = ShellPath(),
      .arguments = {"-c", "exit 7"},
      .tab_id = tab_id,
  });

  status = host.Launch();
  assert(status.ok());
  status = host.WaitForExitFor(std::chrono::seconds(5));
  assert(status.ok());
  assert(host.state() == speed::browser::ChildProcessState::kCrashed);
  assert(host.CrashReason().find("7") != std::string::npos);

  status = browser.HandleRendererCrash(host.tab_id(), host.CrashReason());
  assert(status.ok());
  const speed::browser::TabState* const tab = browser.tabs().GetTabState(tab_id);
  assert(tab != nullptr);
  assert(tab->navigation_state == speed::browser::TabNavigationState::kCrashed);
  assert(tab->last_error.find("7") != std::string::npos);
}

void TerminatesForShutdownWithoutCrashState()
{
  const std::filesystem::path sleep_path = "/bin/sleep";
  if (!std::filesystem::exists(sleep_path))
  {
    return;
  }

  speed::browser::ChildProcessHost host({
      .role = speed::browser::ChildProcessRole::kUtility,
      .executable_path = sleep_path,
      .arguments = {"10"},
      .tab_id = {},
  });

  speed::base::Status status = host.Launch();
  assert(status.ok());
  status = host.TerminateForShutdown(std::chrono::seconds(5));
  assert(status.ok());
  assert(host.state() == speed::browser::ChildProcessState::kExited);
}

} // namespace

int main()
{
  RecordsCleanExit();
  ConvertsCrashToExplicitStateAndTabCrash();
  TerminatesForShutdownWithoutCrashState();
  return 0;
}
