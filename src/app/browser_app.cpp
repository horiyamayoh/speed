#include "app/browser_app.h"

#include "aegis/classifier/request_classifier.h"
#include "base/logging/logging.h"
#include "browser/browser_process.h"
#include "browser/tabs/tab_model.h"
#include "network/network_process.h"
#include "renderer/document/renderer_process.h"
#include "storage/history/history_store.h"
#include "storage/profile/profile_directory.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>

namespace speed::app
{

namespace
{

[[nodiscard]] aegis::RequestClassifier BuildDefaultClassifier(base::Status& status)
{
  aegis::RequestClassifier classifier;
  const base::Status rule_status = classifier.AddBlockingRule({
      .pattern = "ads.example",
      .reason = "blocked by embedded Aegis MVP rule",
  });
  if (!rule_status.ok())
  {
    status = rule_status;
  }
  return classifier;
}

[[nodiscard]] network::NetworkProcess
BuildNetworkProcess(std::unique_ptr<network::FetchAdapter> fetch_adapter, base::Status& status)
{
  aegis::RequestClassifier classifier = BuildDefaultClassifier(status);
  return network::NetworkProcess(
      network::NetworkService(std::move(classifier), std::move(fetch_adapter)));
}

[[nodiscard]] storage::HistoryStore OpenHistoryStore(const std::filesystem::path& profile_root,
                                                     base::Status& status)
{
  storage::HistoryStore history_store;
  const storage::ProfileDirectory profile(profile_root.empty() ? DefaultProfileRoot()
                                                               : profile_root);
  const base::Status history_status = history_store.Open(profile);
  if (!history_status.ok())
  {
    status = history_status;
  }
  return history_store;
}

[[nodiscard]] ui::ShellTabNavigationState MapTabState(browser::TabNavigationState state)
{
  switch (state)
  {
  case browser::TabNavigationState::kEmpty:
    return ui::ShellTabNavigationState::kEmpty;
  case browser::TabNavigationState::kLoading:
    return ui::ShellTabNavigationState::kLoading;
  case browser::TabNavigationState::kCommitted:
    return ui::ShellTabNavigationState::kCommitted;
  case browser::TabNavigationState::kBlocked:
    return ui::ShellTabNavigationState::kBlocked;
  case browser::TabNavigationState::kFailed:
    return ui::ShellTabNavigationState::kFailed;
  case browser::TabNavigationState::kCrashed:
    return ui::ShellTabNavigationState::kCrashed;
  }

  return ui::ShellTabNavigationState::kFailed;
}

} // namespace

class BrowserApp::NetworkClient final : public browser::NavigationNetworkClient
{
public:
  explicit NetworkClient(network::NetworkProcess& process)
      : process_(process)
  {}

  ipc::navigation::NavigateResponse
  SendNavigateRequest(const ipc::navigation::NavigateRequest& request) override
  {
    return process_.HandleNavigateRequest(request);
  }

private:
  network::NetworkProcess& process_;
};

class BrowserApp::RendererClient final : public browser::NavigationRendererClient
{
public:
  explicit RendererClient(renderer::RendererProcess& process)
      : process_(process)
  {}

  base::Status SendCommitDocument(const ipc::navigation::CommitDocument& commit) override
  {
    return process_.CommitDocument(commit);
  }

  base::Status SendCommitErrorPage(const ipc::navigation::CommitErrorPage& commit) override
  {
    return process_.CommitErrorPage(commit);
  }

private:
  renderer::RendererProcess& process_;
};

class BrowserApp::Impl final
{
public:
  explicit Impl(BrowserAppOptions options)
      : initialization_status_(base::Status::Ok()),
        network_process_(
            BuildNetworkProcess(std::move(options.fetch_adapter), initialization_status_)),
        renderer_process_(),
        network_client_(network_process_),
        renderer_client_(renderer_process_),
        browser_process_(network_client_,
                         renderer_client_,
                         OpenHistoryStore(options.profile_root, initialization_status_))
  {}

  [[nodiscard]] base::Status Start()
  {
    if (!initialization_status_.ok())
    {
      return initialization_status_;
    }

    const base::Status start_status = browser_process_.Start();
    if (!start_status.ok())
    {
      return start_status;
    }

    TrackKnownTab(browser_process_.tabs().active_tab());
    return base::Status::Ok();
  }

  [[nodiscard]] bool running() const
  {
    return browser_process_.running();
  }

  [[nodiscard]] base::Status CreateTab(base::TabId& created_tab)
  {
    created_tab = browser_process_.CreateTab();
    if (!created_tab)
    {
      return base::Status::Error("browser process did not create a tab");
    }

    TrackKnownTab(created_tab);
    return base::Status::Ok();
  }

  [[nodiscard]] base::Status SwitchToTab(base::TabId tab_id)
  {
    if (!browser_process_.SwitchToTab(tab_id))
    {
      return base::Status::Error("tab does not exist");
    }

    TrackKnownTab(tab_id);
    return base::Status::Ok();
  }

  [[nodiscard]] base::Status CloseTab(base::TabId tab_id)
  {
    if (!browser_process_.CloseTab(tab_id))
    {
      return base::Status::Error("tab does not exist");
    }

    const auto closed_tab = std::find(known_tabs_.begin(), known_tabs_.end(), tab_id);
    if (closed_tab != known_tabs_.end())
    {
      known_tabs_.erase(closed_tab);
    }

    TrackKnownTab(browser_process_.tabs().active_tab());
    return base::Status::Ok();
  }

  [[nodiscard]] base::Status NavigateActiveTab(std::string_view input)
  {
    return browser_process_.NavigateActiveTab(input);
  }

  [[nodiscard]] ui::BrowserShellSnapshot Snapshot() const
  {
    ui::BrowserShellSnapshot snapshot{
        .active_tab = browser_process_.tabs().active_tab(),
        .tabs = {},
        .history = browser_process_.history().entries(),
        .active_page = std::nullopt,
    };

    for (const base::TabId tab_id : known_tabs_)
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

  [[nodiscard]] base::Status Shutdown()
  {
    if (!browser_process_.running())
    {
      return base::Status::Ok();
    }

    return browser_process_.Shutdown();
  }

private:
  void TrackKnownTab(base::TabId tab_id)
  {
    if (!tab_id || IsKnownTab(tab_id))
    {
      return;
    }

    known_tabs_.push_back(tab_id);
  }

  [[nodiscard]] bool IsKnownTab(base::TabId tab_id) const
  {
    return std::find(known_tabs_.begin(), known_tabs_.end(), tab_id) != known_tabs_.end();
  }

  void AppendTabSnapshot(base::TabId tab_id, ui::BrowserShellSnapshot& snapshot) const
  {
    const browser::TabState* const tab = browser_process_.tabs().GetTabState(tab_id);
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

  void AppendActivePageSnapshot(ui::BrowserShellSnapshot& snapshot) const
  {
    if (!snapshot.active_tab)
    {
      return;
    }

    const std::vector<renderer::CommittedDocument> committed_documents =
        renderer_process_.committed_documents();
    for (auto document = committed_documents.rbegin(); document != committed_documents.rend();
         ++document)
    {
      if (document->tab_id != snapshot.active_tab)
      {
        continue;
      }

      snapshot.active_page = ui::ShellPageSnapshot{
          .tab_id = document->tab_id,
          .document_id = document->document_id,
          .url = document->url,
          .document_body = document->document_body,
          .is_error_page = document->is_error_page,
          .display_list = document->render_result.display_list,
          .content_height = document->render_result.layout.content_height,
      };
      return;
    }
  }

  base::Status initialization_status_;
  network::NetworkProcess network_process_;
  renderer::RendererProcess renderer_process_;
  NetworkClient network_client_;
  RendererClient renderer_client_;
  browser::BrowserProcess browser_process_;
  std::vector<base::TabId> known_tabs_;
};

std::filesystem::path DefaultProfileRoot()
{
  if (const char* const configured_profile = std::getenv("SPEED_PROFILE_DIR");
      configured_profile != nullptr && configured_profile[0] != '\0')
  {
    return configured_profile;
  }

  return std::filesystem::temp_directory_path() / "speed-profile";
}

BrowserApp::BrowserApp()
    : BrowserApp(BrowserAppOptions{})
{}

BrowserApp::BrowserApp(BrowserAppOptions options)
    : impl_(std::make_unique<Impl>(std::move(options)))
{}

BrowserApp::~BrowserApp() = default;

base::Status BrowserApp::Start()
{
  return impl_->Start();
}

bool BrowserApp::running() const
{
  return impl_->running();
}

base::Status BrowserApp::CreateTab(base::TabId& created_tab)
{
  return impl_->CreateTab(created_tab);
}

base::Status BrowserApp::SwitchToTab(base::TabId tab_id)
{
  return impl_->SwitchToTab(tab_id);
}

base::Status BrowserApp::CloseTab(base::TabId tab_id)
{
  return impl_->CloseTab(tab_id);
}

base::Status BrowserApp::NavigateActiveTab(std::string_view input)
{
  return impl_->NavigateActiveTab(input);
}

ui::BrowserShellSnapshot BrowserApp::Snapshot() const
{
  return impl_->Snapshot();
}

base::Status BrowserApp::Shutdown()
{
  return impl_->Shutdown();
}

} // namespace speed::app
