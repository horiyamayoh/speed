#include "app/browser_app.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <thread>

namespace
{

class ScopedRendererCrashEnv final
{
public:
  ScopedRendererCrashEnv()
  {
    const char* const existing = std::getenv("SPEED_RENDERER_CRASH_AFTER_COMMIT_COUNT");
    if (existing != nullptr)
    {
      previous_value_ = existing;
    }
    setenv("SPEED_RENDERER_CRASH_AFTER_COMMIT_COUNT", "1", 1);
  }

  ~ScopedRendererCrashEnv()
  {
    if (previous_value_.empty())
    {
      unsetenv("SPEED_RENDERER_CRASH_AFTER_COMMIT_COUNT");
      return;
    }

    setenv("SPEED_RENDERER_CRASH_AFTER_COMMIT_COUNT", previous_value_.c_str(), 1);
  }

private:
  std::string previous_value_;
};

class TempProfile final
{
public:
  TempProfile()
      : root_(std::filesystem::temp_directory_path() /
              ("speed_renderer_crash_recovery_test_" +
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

[[nodiscard]] bool HasCrashText(const speed::ui::BrowserShellSnapshot& snapshot)
{
  if (!snapshot.active_page.has_value())
  {
    return false;
  }

  return snapshot.active_page->document_body.find("Tab crashed") != std::string::npos &&
         !snapshot.active_page->display_list.commands.empty();
}

} // namespace

int main()
{
  ScopedRendererCrashEnv crash_env;
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

  for (int attempt = 0; attempt < 100; ++attempt)
  {
    snapshot = app.Snapshot();
    const speed::ui::ShellTabSnapshot* const tab = FindTab(snapshot, tab_id);
    if (tab != nullptr && tab->navigation_state == speed::ui::ShellTabNavigationState::kCrashed)
    {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  snapshot = app.Snapshot();
  const speed::ui::ShellTabSnapshot* const tab = FindTab(snapshot, tab_id);
  assert(tab != nullptr);
  assert(tab->navigation_state == speed::ui::ShellTabNavigationState::kCrashed);
  assert(tab->last_error.find("renderer process crashed") != std::string::npos);
  assert(snapshot.active_page.has_value());
  assert(snapshot.active_page->is_error_page);
  assert(HasCrashText(snapshot));
  assert(app.running());

  status = app.Shutdown();
  assert(status.ok());
  return 0;
}
