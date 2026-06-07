#pragma once

#include "base/ids/id_types.h"
#include "base/result/status.h"
#include "engine/paint/display_list.h"

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace speed::ui
{

enum class ShellTabNavigationState
{
  kEmpty,
  kLoading,
  kCommitted,
  kBlocked,
  kFailed,
  kCrashed,
};

struct ShellTabSnapshot final
{
  base::TabId id;
  ShellTabNavigationState navigation_state{ShellTabNavigationState::kEmpty};
  std::string pending_url;
  std::string current_url;
  std::string last_error;
};

struct ShellPageSnapshot final
{
  base::TabId tab_id;
  base::DocumentId document_id;
  std::string url;
  std::string document_body;
  bool is_error_page{false};
  engine::paint::DisplayList display_list;
  int content_height{0};
};

struct BrowserShellSnapshot final
{
  base::TabId active_tab;
  std::vector<ShellTabSnapshot> tabs;
  std::vector<std::string> history;
  std::optional<ShellPageSnapshot> active_page;
};

class BrowserShellDelegate
{
public:
  virtual ~BrowserShellDelegate();

  [[nodiscard]] virtual base::Status CreateTab(base::TabId& created_tab) = 0;
  [[nodiscard]] virtual base::Status SwitchToTab(base::TabId tab_id) = 0;
  [[nodiscard]] virtual base::Status CloseTab(base::TabId tab_id) = 0;
  [[nodiscard]] virtual base::Status NavigateActiveTab(std::string_view input) = 0;
  [[nodiscard]] virtual BrowserShellSnapshot Snapshot() const = 0;
  [[nodiscard]] virtual base::Status Shutdown() = 0;
};

class BrowserShell final
{
public:
  BrowserShell();
  BrowserShell(BrowserShellDelegate& delegate, std::istream& input, std::ostream& output);

  void Show() const;
  [[nodiscard]] int Run();
  [[nodiscard]] base::Status RunSmokeNavigation(std::string_view url);

private:
  [[nodiscard]] bool RunCommand(std::string_view line);
  [[nodiscard]] bool RunNavigationCommand(std::string_view url);
  [[nodiscard]] bool RunNewTabCommand(std::string_view url);
  [[nodiscard]] bool RunSwitchCommand(std::string_view tab_id);
  [[nodiscard]] bool RunCloseCommand(std::string_view tab_id);

  void PrintHelp() const;
  void PrintTabs() const;
  void PrintHistory() const;
  void PrintActivePage() const;
  void PrintActiveError() const;
  void PrintStatus(const base::Status& status) const;

  BrowserShellDelegate* delegate_{nullptr};
  std::istream* input_{nullptr};
  std::ostream* output_{nullptr};
};

} // namespace speed::ui
