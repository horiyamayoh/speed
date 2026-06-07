#pragma once

#include "base/ids/id_types.h"
#include "base/result/status.h"
#include "browser/navigation/navigation_controller.h"
#include "browser/tabs/tab_model.h"
#include "ipc/runtime/navigation_messages.h"
#include "storage/history/history_store.h"

#include <cstdint>
#include <string_view>

namespace speed::browser
{

class NavigationNetworkClient
{
public:
  virtual ~NavigationNetworkClient() = default;

  [[nodiscard]] virtual ipc::navigation::NavigateResponse
  SendNavigateRequest(const ipc::navigation::NavigateRequest& request) = 0;
};

class NavigationRendererClient
{
public:
  virtual ~NavigationRendererClient() = default;

  [[nodiscard]] virtual base::Status
  SendCommitDocument(const ipc::navigation::CommitDocument& commit) = 0;
};

class BrowserProcess final
{
public:
  enum class State : std::uint8_t
  {
    kCreated,
    kRunning,
    kStopped,
  };

  BrowserProcess(NavigationNetworkClient& network_client,
                 NavigationRendererClient& renderer_client);

  [[nodiscard]] base::Status Start();
  [[nodiscard]] base::Status Shutdown();

  [[nodiscard]] base::TabId CreateTab();
  [[nodiscard]] bool SwitchToTab(base::TabId tab_id);
  [[nodiscard]] base::Status NavigateActiveTab(std::string_view input);
  [[nodiscard]] base::Status NavigateTab(base::TabId tab_id, std::string_view input);

  [[nodiscard]] State state() const;
  [[nodiscard]] bool running() const;
  [[nodiscard]] const TabModel& tabs() const;
  [[nodiscard]] const storage::HistoryStore& history() const;

private:
  [[nodiscard]] base::Status
  HandleNavigateResponse(const NavigationRequest& request,
                         const ipc::navigation::NavigateResponse& response);

  NavigationNetworkClient& network_client_;
  NavigationRendererClient& renderer_client_;
  TabModel tabs_;
  NavigationController navigation_;
  storage::HistoryStore history_;
  std::uint64_t next_document_id_{1};
  State state_{State::kCreated};
};

} // namespace speed::browser
