#pragma once

#include "base/result/status.h"
#include "network/fetch/fetch_adapter.h"
#include "ui/shell/browser_shell.h"

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace speed::app
{

struct BrowserAppOptions final
{
  std::filesystem::path profile_root;
  std::unique_ptr<network::FetchAdapter> fetch_adapter;
};

[[nodiscard]] std::filesystem::path DefaultProfileRoot();

class BrowserApp final : public ui::BrowserShellDelegate
{
public:
  BrowserApp();
  explicit BrowserApp(BrowserAppOptions options);
  ~BrowserApp() override;

  BrowserApp(const BrowserApp&) = delete;
  BrowserApp& operator=(const BrowserApp&) = delete;
  BrowserApp(BrowserApp&&) = delete;
  BrowserApp& operator=(BrowserApp&&) = delete;

  [[nodiscard]] base::Status Start();
  [[nodiscard]] bool running() const;

  [[nodiscard]] base::Status CreateTab(base::TabId& created_tab) override;
  [[nodiscard]] base::Status SwitchToTab(base::TabId tab_id) override;
  [[nodiscard]] base::Status CloseTab(base::TabId tab_id) override;
  [[nodiscard]] base::Status NavigateActiveTab(std::string_view input) override;
  [[nodiscard]] ui::BrowserShellSnapshot Snapshot() const override;
  [[nodiscard]] base::Status Shutdown() override;

private:
  class NetworkClient;
  class RendererClient;
  class Impl;

  std::unique_ptr<Impl> impl_;
};

} // namespace speed::app
