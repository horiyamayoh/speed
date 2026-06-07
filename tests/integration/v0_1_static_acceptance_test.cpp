#include "app/browser_app.h"
#include "network/fetch/fetch_adapter.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

namespace
{

class TempProfile final
{
public:
  explicit TempProfile(std::string name)
      : root_(std::filesystem::temp_directory_path() /
              (std::move(name) + "_" +
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

class StaticFetchAdapter final : public speed::network::FetchAdapter
{
public:
  speed::network::FetchResult Fetch(const speed::network::FetchRequest& request) const override
  {
    return speed::network::FetchResult::Success(
        "<html><head><style>"
        "body { background-color: #ffffff; color: #111111; }"
        "p { color: red; font-size: 18px; }"
        "</style></head><body><p>Accepted " +
        request.url + "</p></body></html>");
  }
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

void AcceptsStaticInProcessBrowsing()
{
  TempProfile profile("speed_v0_1_static_acceptance_in_process");
  speed::app::BrowserApp app({
      .profile_root = profile.root(),
      .fetch_adapter = std::make_unique<StaticFetchAdapter>(),
      .process_model = speed::app::BrowserAppProcessModel::kInProcess,
      .app_binary_dir = {},
  });

  speed::base::Status status = app.Start();
  assert(status.ok());
  status = app.NavigateActiveTab("Example.TEST/page");
  assert(status.ok());

  speed::ui::BrowserShellSnapshot snapshot = app.Snapshot();
  const speed::base::TabId first_tab = snapshot.active_tab;
  assert(first_tab);
  assert(snapshot.active_page.has_value());
  assert(!snapshot.active_page->display_list.commands.empty());
  assert(snapshot.history.size() == 2);
  assert(snapshot.history.back() == "https://example.test/page");

  speed::base::TabId second_tab;
  status = app.CreateTab(second_tab);
  assert(status.ok());
  status = app.NavigateActiveTab("Second.TEST/page");
  assert(status.ok());
  snapshot = app.Snapshot();
  assert(snapshot.active_tab == second_tab);
  assert(snapshot.history.size() == 3);
  assert(FindTab(snapshot, first_tab) != nullptr);
  assert(FindTab(snapshot, second_tab) != nullptr);

  status = app.NavigateActiveTab("https://ads.example/tracker");
  assert(!status.ok());
  snapshot = app.Snapshot();
  const speed::ui::ShellTabSnapshot* const tab = FindTab(snapshot, second_tab);
  assert(tab != nullptr);
  assert(tab->navigation_state == speed::ui::ShellTabNavigationState::kBlocked);
  assert(snapshot.history.size() == 3);
  assert(snapshot.active_page.has_value());
  assert(snapshot.active_page->is_error_page);

  status = app.Shutdown();
  assert(status.ok());
}

void AcceptsOptInMultiProcessSmoke()
{
  TempProfile profile("speed_v0_1_static_acceptance_multi_process");
  speed::app::BrowserApp app({
      .profile_root = profile.root(),
      .fetch_adapter = nullptr,
      .process_model = speed::app::BrowserAppProcessModel::kMultiProcess,
      .app_binary_dir = SPEED_APP_DIR,
  });

  speed::base::Status status = app.Start();
  assert(status.ok());
  status = app.NavigateActiveTab("about:blank");
  assert(status.ok());

  speed::ui::BrowserShellSnapshot snapshot = app.Snapshot();
  assert(snapshot.active_tab);
  assert(snapshot.active_page.has_value());
  assert(snapshot.history.size() == 2);

  status = app.NavigateActiveTab("https://ads.example/tracker");
  assert(!status.ok());
  snapshot = app.Snapshot();
  assert(snapshot.active_page.has_value());
  assert(snapshot.active_page->is_error_page);
  const speed::ui::ShellTabSnapshot* const tab = FindTab(snapshot, snapshot.active_tab);
  assert(tab != nullptr);
  assert(tab->navigation_state == speed::ui::ShellTabNavigationState::kBlocked);

  status = app.Shutdown();
  assert(status.ok());
}

} // namespace

int main()
{
  AcceptsStaticInProcessBrowsing();
  AcceptsOptInMultiProcessSmoke();
  return 0;
}
