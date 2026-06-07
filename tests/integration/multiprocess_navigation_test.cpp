#include "app/browser_app.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>

namespace
{

class TempProfile final
{
public:
  TempProfile()
      : root_(std::filesystem::temp_directory_path() /
              ("speed_multiprocess_navigation_test_" +
               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
  {}

  TempProfile(const TempProfile&) = delete;
  TempProfile& operator=(const TempProfile&) = delete;

  ~TempProfile()
  {
    std::error_code ignored;
    std::filesystem::remove_all(root_, ignored);
  }

  [[nodiscard]] const std::filesystem::path& root() const
  {
    return root_;
  }

private:
  std::filesystem::path root_;
};

[[nodiscard]] const speed::ui::ShellTabSnapshot*
FindTab(const speed::ui::BrowserShellSnapshot& snapshot, speed::base::TabId tab_id)
{
  for (const speed::ui::ShellTabSnapshot& tab : snapshot.tabs)
  {
    if (tab.id == tab_id)
    {
      return &tab;
    }
  }

  return nullptr;
}

} // namespace

int main()
{
  TempProfile profile;
  speed::app::BrowserApp app({
      .profile_root = profile.root(),
      .fetch_adapter = nullptr,
      .process_model = speed::app::BrowserAppProcessModel::kMultiProcess,
      .app_binary_dir = SPEED_APP_DIR,
  });

  speed::base::Status status = app.Start();
  assert(status.ok());

  speed::ui::BrowserShellSnapshot snapshot = app.Snapshot();
  const speed::base::TabId tab_id = snapshot.active_tab;
  assert(tab_id);
  assert(snapshot.history.size() == 1);
  assert(snapshot.history.back() == "about:blank");
  assert(snapshot.active_page.has_value());

  status = app.NavigateActiveTab("about:blank");
  assert(status.ok());
  snapshot = app.Snapshot();
  assert(snapshot.history.size() == 2);
  assert(snapshot.active_page.has_value());
  const speed::ui::ShellTabSnapshot* tab = FindTab(snapshot, tab_id);
  assert(tab != nullptr);
  assert(tab->navigation_state == speed::ui::ShellTabNavigationState::kCommitted);

  status = app.NavigateActiveTab("https://ads.example/tracker");
  assert(!status.ok());
  snapshot = app.Snapshot();
  assert(snapshot.history.size() == 2);
  assert(snapshot.active_page.has_value());
  assert(snapshot.active_page->is_error_page);
  assert(!snapshot.active_page->display_list.commands.empty());
  tab = FindTab(snapshot, tab_id);
  assert(tab != nullptr);
  assert(tab->navigation_state == speed::ui::ShellTabNavigationState::kBlocked);
  assert(tab->last_error == "blocked by embedded Aegis MVP rule");

  status = app.Shutdown();
  assert(status.ok());
  return 0;
}
