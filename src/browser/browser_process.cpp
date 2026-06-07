#include "browser/browser_process.h"

#include "base/logging/logging.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace speed::browser
{

namespace
{

constexpr std::string_view kInitialUrl = "about:blank";

[[nodiscard]] bool IsRendererCrashStatus(const base::Status& status)
{
  return status.message().find("renderer process crashed") != std::string::npos ||
         status.message().find("child process exited") != std::string::npos;
}

} // namespace

BrowserProcess::BrowserProcess(NavigationNetworkClient& network_client,
                               NavigationRendererClient& renderer_client,
                               storage::HistoryStore history_store)
    : network_client_(network_client),
      renderer_client_(renderer_client),
      history_(std::move(history_store))
{}

base::Status BrowserProcess::Start()
{
  if (state_ == State::kRunning)
  {
    return base::Status::Error("browser process is already running");
  }

  if (state_ == State::kStopped)
  {
    return base::Status::Error("browser process cannot be restarted after shutdown");
  }

  base::Log(base::LogLevel::kInfo, "browser", "browser process starting");

  state_ = State::kRunning;
  const base::TabId initial_tab = tabs_.CreateTab();
  const base::Status navigation_status = NavigateTab(initial_tab, kInitialUrl);
  if (!navigation_status.ok())
  {
    tabs_.CloseAllTabs();
    state_ = State::kStopped;
    return navigation_status;
  }

  base::Log(base::LogLevel::kInfo, "browser", "browser process ready");
  return base::Status::Ok();
}

base::Status BrowserProcess::Shutdown()
{
  if (state_ != State::kRunning)
  {
    return base::Status::Error("browser process is not running");
  }

  if (history_.durable())
  {
    const base::Status history_status = history_.Flush();
    if (!history_status.ok())
    {
      return history_status;
    }
  }

  tabs_.CloseAllTabs();
  state_ = State::kStopped;
  base::Log(base::LogLevel::kInfo, "browser", "browser process stopped");
  return base::Status::Ok();
}

base::TabId BrowserProcess::CreateTab()
{
  if (state_ != State::kRunning)
  {
    return {};
  }

  return tabs_.CreateTab();
}

bool BrowserProcess::CloseTab(base::TabId tab_id)
{
  if (state_ != State::kRunning)
  {
    return false;
  }

  return tabs_.CloseTab(tab_id);
}

bool BrowserProcess::SwitchToTab(base::TabId tab_id)
{
  if (state_ != State::kRunning)
  {
    return false;
  }

  return tabs_.SwitchToTab(tab_id);
}

base::Status BrowserProcess::NavigateActiveTab(std::string_view input)
{
  const base::TabId tab_id = tabs_.active_tab();
  if (!tab_id)
  {
    return base::Status::Error("no active tab is available");
  }

  return NavigateTab(tab_id, input);
}

base::Status BrowserProcess::NavigateTab(base::TabId tab_id, std::string_view input)
{
  if (state_ != State::kRunning)
  {
    return base::Status::Error("browser process is not running");
  }

  if (!tabs_.ContainsTab(tab_id))
  {
    return base::Status::Error("tab does not exist");
  }

  const std::optional<NavigationRequest> request =
      navigation_.CreateNavigationRequest(tab_id, input);
  if (!request.has_value())
  {
    return base::Status::Error("navigation request is invalid");
  }

  base::Status status =
      tabs_.StartNavigation(tab_id, request->request_id, std::string(request->url));
  if (!status.ok())
  {
    return status;
  }

  const ipc::navigation::NavigateResponse response = network_client_.SendNavigateRequest({
      .request_id = request->request_id,
      .tab_id = request->tab_id,
      .url = request->url,
      .is_top_level = true,
  });

  return HandleNavigateResponse(*request, response);
}

base::Status BrowserProcess::HandleRendererCrash(base::TabId tab_id, std::string_view reason)
{
  if (state_ != State::kRunning)
  {
    return base::Status::Error("browser process is not running");
  }

  if (!tabs_.ContainsTab(tab_id))
  {
    return base::Status::Error("tab does not exist");
  }

  const std::string message = reason.empty() ? "renderer crashed" : std::string(reason);
  return tabs_.MarkCrashed(tab_id, message);
}

BrowserProcess::State BrowserProcess::state() const
{
  return state_;
}

bool BrowserProcess::running() const
{
  return state_ == State::kRunning;
}

const TabModel& BrowserProcess::tabs() const
{
  return tabs_;
}

const storage::HistoryStore& BrowserProcess::history() const
{
  return history_;
}

base::Status
BrowserProcess::HandleNavigateResponse(const NavigationRequest& request,
                                       const ipc::navigation::NavigateResponse& response)
{
  if (response.request_id != request.request_id)
  {
    (void)tabs_.FailNavigation(
        request.tab_id, request.request_id, "navigation response request id mismatch");
    return base::Status::Error("navigation response request id mismatch");
  }

  if (!ipc::navigation::IsValidNavigateResponse(response))
  {
    (void)tabs_.FailNavigation(request.tab_id, request.request_id, "invalid navigation response");
    return base::Status::Error("invalid navigation response");
  }

  switch (response.status)
  {
  case ipc::navigation::NavigateStatus::kAllowed:
  {
    const base::DocumentId document_id = base::DocumentId::FromRaw(next_document_id_++);
    const ipc::navigation::CommitDocument commit = {
        .tab_id = request.tab_id,
        .document_id = document_id,
        .url = request.url,
        .document_body = response.document_body,
    };

    base::Status status = renderer_client_.SendCommitDocument(commit);
    if (!status.ok())
    {
      if (IsRendererCrashStatus(status))
      {
        (void)tabs_.MarkCrashed(request.tab_id, status.message());
        return status;
      }

      (void)tabs_.FailNavigation(request.tab_id, request.request_id, status.message());
      return status;
    }

    status = tabs_.CommitNavigation(
        request.tab_id, request.request_id, document_id, std::string(request.url));
    if (!status.ok())
    {
      return status;
    }

    status = history_.RecordVisit(request.url);
    if (!status.ok())
    {
      return status;
    }

    if (history_.durable())
    {
      return history_.Flush();
    }

    return base::Status::Ok();
  }
  case ipc::navigation::NavigateStatus::kBlocked:
  {
    const std::string reason =
        response.aegis_reason.empty() ? "navigation blocked" : response.aegis_reason;
    const base::Status error_page_status = CommitErrorPage(
        request.tab_id, request.url, ipc::navigation::ErrorPageReason::kBlocked, reason);
    if (!error_page_status.ok())
    {
      if (IsRendererCrashStatus(error_page_status))
      {
        (void)tabs_.MarkCrashed(request.tab_id, error_page_status.message());
        return error_page_status;
      }

      (void)tabs_.FailNavigation(request.tab_id, request.request_id, error_page_status.message());
      return error_page_status;
    }

    const base::Status status = tabs_.BlockNavigation(request.tab_id, request.request_id, reason);
    if (!status.ok())
    {
      return status;
    }

    return base::Status::Error("navigation blocked: " + reason);
  }
  case ipc::navigation::NavigateStatus::kFailed:
  {
    const std::string reason =
        response.error_message.empty() ? "navigation failed" : response.error_message;
    const base::Status error_page_status = CommitErrorPage(
        request.tab_id, request.url, ipc::navigation::ErrorPageReason::kFailed, reason);
    if (!error_page_status.ok())
    {
      if (IsRendererCrashStatus(error_page_status))
      {
        (void)tabs_.MarkCrashed(request.tab_id, error_page_status.message());
        return error_page_status;
      }

      (void)tabs_.FailNavigation(request.tab_id, request.request_id, error_page_status.message());
      return error_page_status;
    }

    const base::Status status = tabs_.FailNavigation(request.tab_id, request.request_id, reason);
    if (!status.ok())
    {
      return status;
    }

    return base::Status::Error("navigation failed: " + reason);
  }
  }

  (void)tabs_.FailNavigation(request.tab_id, request.request_id, "unknown navigation response");
  return base::Status::Error("unknown navigation response");
}

base::Status BrowserProcess::CommitErrorPage(base::TabId tab_id,
                                             std::string_view url,
                                             ipc::navigation::ErrorPageReason reason,
                                             std::string_view message)
{
  const ipc::navigation::CommitErrorPage commit = {
      .tab_id = tab_id,
      .document_id = base::DocumentId::FromRaw(next_document_id_++),
      .url = std::string(url),
      .reason = reason,
      .message = std::string(message),
  };

  return renderer_client_.SendCommitErrorPage(commit);
}

} // namespace speed::browser
