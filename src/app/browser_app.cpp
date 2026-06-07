#include "app/browser_app.h"

#include "aegis/classifier/request_classifier.h"
#include "base/logging/logging.h"
#include "browser/browser_process.h"
#include "browser/process_host/child_process_host.h"
#include "browser/tabs/tab_model.h"
#include "engine/render_pipeline.h"
#include "ipc/runtime/framed_transport.h"
#include "ipc/runtime/navigation_codec.h"
#include "network/network_process.h"
#include "renderer/document/renderer_process.h"
#include "storage/history/history_store.h"
#include "storage/profile/profile_directory.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
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

struct AppCommittedDocument final
{
  base::TabId tab_id;
  base::DocumentId document_id;
  std::string url;
  std::string document_body;
  bool is_error_page{false};
  engine::paint::DisplayList display_list;
  int content_height{0};
};

class PageSnapshotSource
{
public:
  virtual ~PageSnapshotSource() = default;

  [[nodiscard]] virtual std::vector<AppCommittedDocument> committed_documents() const = 0;
  virtual void CloseTab(base::TabId tab_id) = 0;
  virtual void RecordCrashPage(base::TabId tab_id,
                               base::DocumentId document_id,
                               std::string url,
                               std::string reason) = 0;
};

struct RendererCrash final
{
  base::TabId tab_id;
  std::string reason;
};

class RendererCrashSource
{
public:
  virtual ~RendererCrashSource() = default;

  [[nodiscard]] virtual std::vector<RendererCrash> TakeRendererCrashes() = 0;
};

[[nodiscard]] static AppCommittedDocument BuildLocalCrashDocument(base::TabId tab_id,
                                                                  base::DocumentId document_id,
                                                                  std::string url,
                                                                  std::string reason)
{
  const std::string body =
      "<html><body><h1>Tab crashed</h1><p>" + reason + "</p><p>" + url + "</p></body></html>";
  const engine::RenderPipeline pipeline;
  const engine::RenderResult render_result =
      pipeline.RenderHtml(body, {.width = 800, .height = 600});
  return {
      .tab_id = tab_id,
      .document_id = document_id ? document_id : base::DocumentId::FromRaw(1),
      .url = std::move(url),
      .document_body = body,
      .is_error_page = true,
      .display_list = render_result.display_list,
      .content_height = render_result.layout.content_height,
  };
}

[[nodiscard]] static engine::paint::DisplayCommandType
MapDisplayCommandType(ipc::navigation::RenderCommandType type)
{
  switch (type)
  {
  case ipc::navigation::RenderCommandType::kRect:
    return engine::paint::DisplayCommandType::kRect;
  case ipc::navigation::RenderCommandType::kText:
    return engine::paint::DisplayCommandType::kText;
  case ipc::navigation::RenderCommandType::kBorder:
    return engine::paint::DisplayCommandType::kBorder;
  case ipc::navigation::RenderCommandType::kImagePlaceholder:
    return engine::paint::DisplayCommandType::kImagePlaceholder;
  }

  return engine::paint::DisplayCommandType::kRect;
}

[[nodiscard]] static engine::paint::DisplayCommand
MapDisplayCommand(const ipc::navigation::RenderDisplayCommand& command)
{
  return {
      .type = MapDisplayCommandType(command.type),
      .rect =
          {
              .x = command.x,
              .y = command.y,
              .width = command.width,
              .height = command.height,
          },
      .color =
          {
              .red = command.color_red,
              .green = command.color_green,
              .blue = command.color_blue,
              .alpha = command.color_alpha,
          },
      .border_width =
          {
              .top = command.border_top,
              .right = command.border_right,
              .bottom = command.border_bottom,
              .left = command.border_left,
          },
      .font_size_px = command.font_size_px,
      .text = command.text,
  };
}

[[nodiscard]] static engine::paint::DisplayList
MapDisplayList(const std::vector<ipc::navigation::RenderDisplayCommand>& commands)
{
  engine::paint::DisplayList display_list;
  display_list.commands.reserve(commands.size());
  for (const ipc::navigation::RenderDisplayCommand& command : commands)
  {
    display_list.commands.push_back(MapDisplayCommand(command));
  }
  return display_list;
}

[[nodiscard]] static AppCommittedDocument
MapCommittedDocument(const renderer::CommittedDocument& document)
{
  return {
      .tab_id = document.tab_id,
      .document_id = document.document_id,
      .url = document.url,
      .document_body = document.document_body,
      .is_error_page = document.is_error_page,
      .display_list = document.render_result.display_list,
      .content_height = document.render_result.layout.content_height,
  };
}

[[nodiscard]] static ipc::navigation::NavigateResponse
FailedNavigateResponse(const ipc::navigation::NavigateRequest& request, std::string message)
{
  return {
      .request_id = request.request_id,
      .status = ipc::navigation::NavigateStatus::kFailed,
      .aegis_reason = {},
      .error_message = std::move(message),
      .document_body = {},
  };
}

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

class IpcNetworkClient final : public browser::NavigationNetworkClient
{
public:
  IpcNetworkClient(const std::filesystem::path& app_binary_dir, base::Status& status)
  {
    status = Start(app_binary_dir);
  }

  ~IpcNetworkClient() override
  {
    (void)host_.TerminateForShutdown(std::chrono::seconds(1));
  }

  ipc::navigation::NavigateResponse
  SendNavigateRequest(const ipc::navigation::NavigateRequest& request) override
  {
    base::Status status = host_.PollForExit();
    if (!status.ok())
    {
      return FailedNavigateResponse(request, status.message());
    }

    if (host_.state() == browser::ChildProcessState::kCrashed)
    {
      return FailedNavigateResponse(request, host_.CrashReason());
    }

    status = transport_.SendMessage(ipc::navigation::EncodeNavigateRequest(request));
    if (!status.ok())
    {
      return FailedNavigateResponse(request, status.message());
    }

    ipc::Message message({}, {});
    status = transport_.ReceiveMessage(message);
    if (!status.ok())
    {
      return FailedNavigateResponse(request, status.message());
    }

    ipc::navigation::NavigateResponse response;
    status = ipc::navigation::DecodeNavigateResponse(message, response);
    if (!status.ok())
    {
      return FailedNavigateResponse(request, status.message());
    }

    return response;
  }

private:
  [[nodiscard]] base::Status Start(const std::filesystem::path& app_binary_dir)
  {
    ipc::LocalTransportPair pair;
    base::Status status = ipc::CreateLocalTransportPair(pair);
    if (!status.ok())
    {
      return status;
    }

    host_ = browser::ChildProcessHost({
        .role = browser::ChildProcessRole::kNetwork,
        .executable_path = app_binary_dir / "speed-network",
        .arguments = {"--ipc-fd=" + std::to_string(pair.second.file_descriptor())},
        .tab_id = {},
    });
    status = host_.Launch();
    if (!status.ok())
    {
      return status;
    }

    pair.second = ipc::FileDescriptorTransport();
    transport_ = std::move(pair.first);
    return base::Status::Ok();
  }

  browser::ChildProcessHost host_;
  ipc::FileDescriptorTransport transport_;
};

class BrowserApp::RendererClient final : public browser::NavigationRendererClient,
                                         public PageSnapshotSource
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

  std::vector<AppCommittedDocument> committed_documents() const override
  {
    std::vector<AppCommittedDocument> documents;
    const std::vector<renderer::CommittedDocument> committed_documents =
        process_.committed_documents();
    documents.reserve(committed_documents.size());
    for (const renderer::CommittedDocument& document : committed_documents)
    {
      documents.push_back(MapCommittedDocument(document));
    }
    return documents;
  }

  void CloseTab(base::TabId /*tab_id*/) override {}

  void RecordCrashPage(base::TabId /*tab_id*/,
                       base::DocumentId /*document_id*/,
                       std::string /*url*/,
                       std::string /*reason*/) override
  {}

private:
  renderer::RendererProcess& process_;
};

class IpcRendererClient final : public browser::NavigationRendererClient,
                                public PageSnapshotSource,
                                public RendererCrashSource
{
public:
  explicit IpcRendererClient(std::filesystem::path app_binary_dir)
      : app_binary_dir_(std::move(app_binary_dir))
  {}

  ~IpcRendererClient() override
  {
    for (RendererConnection& connection : renderers_)
    {
      (void)connection.host.TerminateForShutdown(std::chrono::seconds(1));
    }
  }

  base::Status SendCommitDocument(const ipc::navigation::CommitDocument& commit) override
  {
    ipc::navigation::RenderReady ready;
    base::Status status =
        SendCommitMessage(commit.tab_id, ipc::navigation::EncodeCommitDocument(commit), ready);
    if (!status.ok())
    {
      RecordCrashDocumentIfNeeded(commit.tab_id, commit.document_id, commit.url, status);
      return status;
    }

    if (!ready.ok)
    {
      status = base::Status::Error(ready.error_message);
      RecordCrashDocumentIfNeeded(commit.tab_id, commit.document_id, commit.url, status);
      return status;
    }

    committed_documents_.push_back({
        .tab_id = commit.tab_id,
        .document_id = commit.document_id,
        .url = commit.url,
        .document_body = commit.document_body,
        .is_error_page = false,
        .display_list = MapDisplayList(ready.display_commands),
        .content_height = ready.content_height,
    });
    return base::Status::Ok();
  }

  base::Status SendCommitErrorPage(const ipc::navigation::CommitErrorPage& commit) override
  {
    ipc::navigation::RenderReady ready;
    base::Status status =
        SendCommitMessage(commit.tab_id, ipc::navigation::EncodeCommitErrorPage(commit), ready);
    if (!status.ok())
    {
      RecordCrashDocumentIfNeeded(commit.tab_id, commit.document_id, commit.url, status);
      return status;
    }

    if (!ready.ok)
    {
      status = base::Status::Error(ready.error_message);
      RecordCrashDocumentIfNeeded(commit.tab_id, commit.document_id, commit.url, status);
      return status;
    }

    committed_documents_.push_back({
        .tab_id = commit.tab_id,
        .document_id = commit.document_id,
        .url = commit.url,
        .document_body = commit.message,
        .is_error_page = true,
        .display_list = MapDisplayList(ready.display_commands),
        .content_height = ready.content_height,
    });
    return base::Status::Ok();
  }

  std::vector<AppCommittedDocument> committed_documents() const override
  {
    return committed_documents_;
  }

  void CloseTab(base::TabId tab_id) override
  {
    const auto connection = std::ranges::find(renderers_, tab_id, &RendererConnection::tab_id);
    if (connection != renderers_.end())
    {
      (void)connection->host.TerminateForShutdown(std::chrono::seconds(1));
      renderers_.erase(connection);
    }

    std::erase_if(committed_documents_,
                  [tab_id](const AppCommittedDocument& document)
                  { return document.tab_id == tab_id; });
  }

  void RecordCrashPage(base::TabId tab_id,
                       base::DocumentId document_id,
                       std::string url,
                       std::string reason) override
  {
    committed_documents_.push_back(
        BuildLocalCrashDocument(tab_id, document_id, std::move(url), std::move(reason)));
  }

  std::vector<RendererCrash> TakeRendererCrashes() override
  {
    std::vector<RendererCrash> crashes;
    for (RendererConnection& connection : renderers_)
    {
      if (connection.reported_crash)
      {
        continue;
      }

      (void)connection.host.PollForExit();
      if (connection.host.state() == browser::ChildProcessState::kCrashed)
      {
        connection.reported_crash = true;
        crashes.push_back({
            .tab_id = connection.tab_id,
            .reason = "renderer process crashed: " + connection.host.CrashReason(),
        });
      }
    }

    return crashes;
  }

private:
  struct RendererConnection final
  {
    base::TabId tab_id;
    browser::ChildProcessHost host;
    ipc::FileDescriptorTransport transport;
    bool reported_crash{false};
  };

  [[nodiscard]] base::Status SendCommitMessage(base::TabId tab_id,
                                               const ipc::Message& message,
                                               ipc::navigation::RenderReady& ready)
  {
    RendererConnection* const connection = EnsureRenderer(tab_id);
    if (connection == nullptr)
    {
      return base::Status::Error("failed to start renderer process");
    }

    base::Status status = connection->host.PollForExit();
    if (!status.ok())
    {
      return status;
    }

    if (connection->host.state() == browser::ChildProcessState::kCrashed)
    {
      connection->reported_crash = true;
      return base::Status::Error("renderer process crashed: " + connection->host.CrashReason());
    }

    status = connection->transport.SendMessage(message);
    if (!status.ok())
    {
      (void)connection->host.PollForExit();
      if (connection->host.state() == browser::ChildProcessState::kCrashed)
      {
        connection->reported_crash = true;
        return base::Status::Error("renderer process crashed: " + connection->host.CrashReason());
      }
      return status;
    }

    ipc::Message response_message({}, {});
    status = connection->transport.ReceiveMessage(response_message);
    if (!status.ok())
    {
      (void)connection->host.PollForExit();
      if (connection->host.state() == browser::ChildProcessState::kCrashed)
      {
        connection->reported_crash = true;
        return base::Status::Error("renderer process crashed: " + connection->host.CrashReason());
      }
      return status;
    }

    return ipc::navigation::DecodeRenderReady(response_message, ready);
  }

  [[nodiscard]] RendererConnection* EnsureRenderer(base::TabId tab_id)
  {
    const auto existing = std::ranges::find(renderers_, tab_id, &RendererConnection::tab_id);
    if (existing != renderers_.end())
    {
      return &*existing;
    }

    ipc::LocalTransportPair pair;
    base::Status status = ipc::CreateLocalTransportPair(pair);
    if (!status.ok())
    {
      return nullptr;
    }

    std::vector<std::string> arguments = {"--ipc-fd=" +
                                          std::to_string(pair.second.file_descriptor())};
    if (const char* const crash_after_count =
            std::getenv("SPEED_RENDERER_CRASH_AFTER_COMMIT_COUNT");
        crash_after_count != nullptr && crash_after_count[0] != '\0')
    {
      arguments.push_back("--crash-after-commit-count=" + std::string(crash_after_count));
    }

    browser::ChildProcessHost host({
        .role = browser::ChildProcessRole::kRenderer,
        .executable_path = app_binary_dir_ / "speed-renderer",
        .arguments = std::move(arguments),
        .tab_id = tab_id,
    });
    status = host.Launch();
    if (!status.ok())
    {
      return nullptr;
    }

    pair.second = ipc::FileDescriptorTransport();
    renderers_.push_back({
        .tab_id = tab_id,
        .host = std::move(host),
        .transport = std::move(pair.first),
        .reported_crash = false,
    });
    return &renderers_.back();
  }

  void RecordCrashDocumentIfNeeded(base::TabId tab_id,
                                   base::DocumentId document_id,
                                   const std::string& url,
                                   const base::Status& status)
  {
    if (status.ok() || status.message().find("renderer process crashed") == std::string::npos)
    {
      return;
    }

    RecordCrashPage(tab_id, document_id, url, status.message());
  }

  std::filesystem::path app_binary_dir_;
  std::vector<RendererConnection> renderers_;
  std::vector<AppCommittedDocument> committed_documents_;
};

class BrowserApp::Impl final
{
public:
  explicit Impl(BrowserAppOptions options)
      : initialization_status_(base::Status::Ok())
  {
    if (options.process_model == BrowserAppProcessModel::kMultiProcess)
    {
      InitializeMultiProcess(std::move(options));
    }
    else
    {
      InitializeInProcess(std::move(options));
    }
  }

  [[nodiscard]] base::Status Start()
  {
    if (!initialization_status_.ok())
    {
      return initialization_status_;
    }

    if (!browser_process_)
    {
      return base::Status::Error("browser process is not initialized");
    }

    const base::Status start_status = browser_process_->Start();
    if (!start_status.ok())
    {
      return start_status;
    }

    TrackKnownTab(browser_process_->tabs().active_tab());
    return base::Status::Ok();
  }

  [[nodiscard]] bool running() const
  {
    return browser_process_ && browser_process_->running();
  }

  [[nodiscard]] base::Status CreateTab(base::TabId& created_tab)
  {
    created_tab = browser_process_->CreateTab();
    if (!created_tab)
    {
      return base::Status::Error("browser process did not create a tab");
    }

    TrackKnownTab(created_tab);
    return base::Status::Ok();
  }

  [[nodiscard]] base::Status SwitchToTab(base::TabId tab_id)
  {
    if (!browser_process_->SwitchToTab(tab_id))
    {
      return base::Status::Error("tab does not exist");
    }

    TrackKnownTab(tab_id);
    return base::Status::Ok();
  }

  [[nodiscard]] base::Status CloseTab(base::TabId tab_id)
  {
    if (!browser_process_->CloseTab(tab_id))
    {
      return base::Status::Error("tab does not exist");
    }

    if (page_source_ != nullptr)
    {
      page_source_->CloseTab(tab_id);
    }

    const auto closed_tab = std::find(known_tabs_.begin(), known_tabs_.end(), tab_id);
    if (closed_tab != known_tabs_.end())
    {
      known_tabs_.erase(closed_tab);
    }

    TrackKnownTab(browser_process_->tabs().active_tab());
    return base::Status::Ok();
  }

  [[nodiscard]] base::Status NavigateActiveTab(std::string_view input)
  {
    return browser_process_->NavigateActiveTab(input);
  }

  [[nodiscard]] ui::BrowserShellSnapshot Snapshot()
  {
    PollRendererCrashes();

    ui::BrowserShellSnapshot snapshot{
        .active_tab = browser_process_->tabs().active_tab(),
        .tabs = {},
        .history = browser_process_->history().entries(),
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
    if (!browser_process_ || !browser_process_->running())
    {
      return base::Status::Ok();
    }

    return browser_process_->Shutdown();
  }

private:
  void InitializeInProcess(BrowserAppOptions options)
  {
    network_process_ = std::make_unique<network::NetworkProcess>(
        BuildNetworkProcess(std::move(options.fetch_adapter), initialization_status_));
    renderer_process_ = std::make_unique<renderer::RendererProcess>();

    auto network_client = std::make_unique<NetworkClient>(*network_process_);
    auto renderer_client = std::make_unique<RendererClient>(*renderer_process_);
    page_source_ = renderer_client.get();
    network_client_ = std::move(network_client);
    renderer_client_ = std::move(renderer_client);
    browser_process_ = std::make_unique<browser::BrowserProcess>(
        *network_client_,
        *renderer_client_,
        OpenHistoryStore(options.profile_root, initialization_status_));
  }

  void InitializeMultiProcess(BrowserAppOptions options)
  {
    const std::filesystem::path app_binary_dir =
        options.app_binary_dir.empty() ? DefaultAppBinaryDir() : options.app_binary_dir;
    auto network_client =
        std::make_unique<IpcNetworkClient>(app_binary_dir, initialization_status_);
    auto renderer_client = std::make_unique<IpcRendererClient>(app_binary_dir);
    page_source_ = renderer_client.get();
    crash_source_ = renderer_client.get();
    network_client_ = std::move(network_client);
    renderer_client_ = std::move(renderer_client);
    browser_process_ = std::make_unique<browser::BrowserProcess>(
        *network_client_,
        *renderer_client_,
        OpenHistoryStore(options.profile_root, initialization_status_));
  }

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
    const browser::TabState* const tab = browser_process_->tabs().GetTabState(tab_id);
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

    if (page_source_ == nullptr)
    {
      return;
    }

    const std::vector<AppCommittedDocument> committed_documents =
        page_source_->committed_documents();
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
          .display_list = document->display_list,
          .content_height = document->content_height,
      };
      return;
    }
  }

  void PollRendererCrashes()
  {
    if (crash_source_ == nullptr || browser_process_ == nullptr || !browser_process_->running())
    {
      return;
    }

    for (const RendererCrash& crash : crash_source_->TakeRendererCrashes())
    {
      const browser::TabState* const tab = browser_process_->tabs().GetTabState(crash.tab_id);
      if (tab == nullptr || tab->navigation_state == browser::TabNavigationState::kCrashed)
      {
        continue;
      }

      if (page_source_ != nullptr)
      {
        const std::string url = tab->current_url.empty() ? tab->pending_url : tab->current_url;
        const base::DocumentId document_id =
            tab->current_document_id ? tab->current_document_id : base::DocumentId::FromRaw(1);
        page_source_->RecordCrashPage(crash.tab_id, document_id, url, crash.reason);
      }

      (void)browser_process_->HandleRendererCrash(crash.tab_id, crash.reason);
    }
  }

  base::Status initialization_status_;
  std::unique_ptr<network::NetworkProcess> network_process_;
  std::unique_ptr<renderer::RendererProcess> renderer_process_;
  std::unique_ptr<browser::NavigationNetworkClient> network_client_;
  std::unique_ptr<browser::NavigationRendererClient> renderer_client_;
  PageSnapshotSource* page_source_{nullptr};
  RendererCrashSource* crash_source_{nullptr};
  std::unique_ptr<browser::BrowserProcess> browser_process_;
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

std::filesystem::path DefaultAppBinaryDir()
{
  std::error_code error;
  const std::filesystem::path self = std::filesystem::read_symlink("/proc/self/exe", error);
  if (!error && !self.empty())
  {
    return self.parent_path();
  }

  return std::filesystem::current_path(error);
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
