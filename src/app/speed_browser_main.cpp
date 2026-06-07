#include "aegis/classifier/request_classifier.h"
#include "base/logging/logging.h"
#include "browser/browser_process.h"
#include "browser/tabs/tab_model.h"
#include "network/network_process.h"
#include "renderer/document/renderer_process.h"
#include "storage/history/history_store.h"
#include "storage/profile/profile_directory.h"
#include "ui/shell/browser_shell.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
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

  speed::base::Status
  SendCommitErrorPage(const speed::ipc::navigation::CommitErrorPage& commit) override
  {
    return process_.CommitErrorPage(commit);
  }

private:
  speed::renderer::RendererProcess& process_;
};

class BrowserShellAdapter final : public speed::ui::BrowserShellDelegate
{
public:
  BrowserShellAdapter(speed::browser::BrowserProcess& browser_process,
                      speed::renderer::RendererProcess& renderer_process)
      : browser_process_(browser_process),
        renderer_process_(renderer_process)
  {
    TrackKnownTab(browser_process_.tabs().active_tab());
  }

  [[nodiscard]] speed::base::Status CreateTab(speed::base::TabId& created_tab) override
  {
    created_tab = browser_process_.CreateTab();
    if (!created_tab)
    {
      return speed::base::Status::Error("browser process did not create a tab");
    }

    TrackKnownTab(created_tab);
    return speed::base::Status::Ok();
  }

  [[nodiscard]] speed::base::Status SwitchToTab(speed::base::TabId tab_id) override
  {
    if (!browser_process_.SwitchToTab(tab_id))
    {
      return speed::base::Status::Error("tab does not exist");
    }

    TrackKnownTab(tab_id);
    return speed::base::Status::Ok();
  }

  [[nodiscard]] speed::base::Status CloseTab(speed::base::TabId tab_id) override
  {
    if (!browser_process_.CloseTab(tab_id))
    {
      return speed::base::Status::Error("tab does not exist");
    }

    const auto closed_tab = std::find(known_tabs_.begin(), known_tabs_.end(), tab_id);
    if (closed_tab != known_tabs_.end())
    {
      known_tabs_.erase(closed_tab);
    }

    TrackKnownTab(browser_process_.tabs().active_tab());
    return speed::base::Status::Ok();
  }

  [[nodiscard]] speed::base::Status NavigateActiveTab(std::string_view input) override
  {
    return browser_process_.NavigateActiveTab(input);
  }

  [[nodiscard]] speed::ui::BrowserShellSnapshot Snapshot() const override
  {
    speed::ui::BrowserShellSnapshot snapshot{
        .active_tab = browser_process_.tabs().active_tab(),
        .tabs = {},
        .history = browser_process_.history().entries(),
        .active_page = std::nullopt,
    };

    for (const speed::base::TabId tab_id : known_tabs_)
    {
      AppendTabSnapshot(tab_id, snapshot);
    }

    if (snapshot.active_tab && !IsKnownTab(snapshot.active_tab))
    {
      AppendTabSnapshot(snapshot.active_tab, snapshot);
    }

    AppendActivePageSnapshot(snapshot);
    return snapshot;
  }

  [[nodiscard]] speed::base::Status Shutdown() override
  {
    if (!browser_process_.running())
    {
      return speed::base::Status::Ok();
    }

    return browser_process_.Shutdown();
  }

private:
  [[nodiscard]] static speed::ui::ShellTabNavigationState
  MapTabState(speed::browser::TabNavigationState state)
  {
    switch (state)
    {
    case speed::browser::TabNavigationState::kEmpty:
      return speed::ui::ShellTabNavigationState::kEmpty;
    case speed::browser::TabNavigationState::kLoading:
      return speed::ui::ShellTabNavigationState::kLoading;
    case speed::browser::TabNavigationState::kCommitted:
      return speed::ui::ShellTabNavigationState::kCommitted;
    case speed::browser::TabNavigationState::kBlocked:
      return speed::ui::ShellTabNavigationState::kBlocked;
    case speed::browser::TabNavigationState::kFailed:
      return speed::ui::ShellTabNavigationState::kFailed;
    case speed::browser::TabNavigationState::kCrashed:
      return speed::ui::ShellTabNavigationState::kCrashed;
    }

    return speed::ui::ShellTabNavigationState::kFailed;
  }

  void TrackKnownTab(speed::base::TabId tab_id)
  {
    if (!tab_id || IsKnownTab(tab_id))
    {
      return;
    }

    known_tabs_.push_back(tab_id);
  }

  [[nodiscard]] bool IsKnownTab(speed::base::TabId tab_id) const
  {
    return std::find(known_tabs_.begin(), known_tabs_.end(), tab_id) != known_tabs_.end();
  }

  void AppendTabSnapshot(speed::base::TabId tab_id, speed::ui::BrowserShellSnapshot& snapshot) const
  {
    const speed::browser::TabState* const tab = browser_process_.tabs().GetTabState(tab_id);
    if (tab == nullptr)
    {
      return;
    }

    snapshot.tabs.push_back({
        .id = tab->id,
        .navigation_state = MapTabState(tab->navigation_state),
        .pending_url = tab->pending_url,
        .current_url = tab->current_url,
        .last_error = tab->last_error,
    });
  }

  void AppendActivePageSnapshot(speed::ui::BrowserShellSnapshot& snapshot) const
  {
    if (!snapshot.active_tab)
    {
      return;
    }

    const std::vector<speed::renderer::CommittedDocument> committed_documents =
        renderer_process_.committed_documents();
    for (auto document = committed_documents.rbegin(); document != committed_documents.rend();
         ++document)
    {
      if (document->tab_id != snapshot.active_tab)
      {
        continue;
      }

      snapshot.active_page = speed::ui::ShellPageSnapshot{
          .tab_id = document->tab_id,
          .document_id = document->document_id,
          .url = document->url,
          .document_body = document->document_body,
          .is_error_page = document->is_error_page,
      };
      return;
    }
  }

  speed::browser::BrowserProcess& browser_process_;
  speed::renderer::RendererProcess& renderer_process_;
  std::vector<speed::base::TabId> known_tabs_;
};

[[nodiscard]] bool IsHelpArgument(std::string_view argument)
{
  return argument == "-h" || argument == "--help";
}

void PrintUsage(std::ostream& output)
{
  output << "usage: speed-browser [url]\n";
  output << "Run without a URL for the interactive console shell.\n";
}

[[nodiscard]] std::filesystem::path DefaultProfileRoot()
{
  if (const char* const configured_profile = std::getenv("SPEED_PROFILE_DIR");
      configured_profile != nullptr && configured_profile[0] != '\0')
  {
    return configured_profile;
  }

  return std::filesystem::temp_directory_path() / "speed-profile";
}

} // namespace

int main(int argc, char* argv[])
{
  if (argc > 2)
  {
    PrintUsage(std::cerr);
    return 1;
  }

  if (argc == 2 && IsHelpArgument(argv[1]))
  {
    PrintUsage(std::cout);
    return 0;
  }

  speed::aegis::RequestClassifier classifier;
  const speed::base::Status rule_status = classifier.AddBlockingRule({
      .pattern = "ads.example",
      .reason = "blocked by embedded Aegis MVP rule",
  });
  if (!rule_status.ok())
  {
    speed::base::Log(speed::base::LogLevel::kError, "browser", rule_status.message());
    return 1;
  }

  speed::storage::HistoryStore history_store;
  const speed::storage::ProfileDirectory profile(DefaultProfileRoot());
  const speed::base::Status history_status = history_store.Open(profile);
  if (!history_status.ok())
  {
    speed::base::Log(speed::base::LogLevel::kError, "browser", history_status.message());
    return 1;
  }

  speed::network::NetworkProcess network_process(
      speed::network::NetworkService(std::move(classifier)));
  speed::renderer::RendererProcess renderer_process;
  InMemoryNetworkClient network_client(network_process);
  InMemoryRendererClient renderer_client(renderer_process);

  speed::browser::BrowserProcess browser_process(
      network_client, renderer_client, std::move(history_store));
  const speed::base::Status start_status = browser_process.Start();
  if (!start_status.ok())
  {
    speed::base::Log(speed::base::LogLevel::kError, "browser", start_status.message());
    return 1;
  }

  BrowserShellAdapter shell_adapter(browser_process, renderer_process);
  speed::ui::BrowserShell shell(shell_adapter, std::cin, std::cout);
  if (argc == 2)
  {
    const speed::base::Status smoke_status = shell.RunSmokeNavigation(argv[1]);
    return smoke_status.ok() ? 0 : 1;
  }

  return shell.Run();
}
