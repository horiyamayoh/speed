#include "app/browser_app.h"
#include "network/fetch/fetch_adapter.h"
#include "ui/gui/display_list_renderer.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace
{

class TempProfile final
{
public:
  TempProfile()
      : root_(std::filesystem::temp_directory_path() /
              ("speed_gui_app_smoke_test_" +
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
        "div { border: 2px solid blue; background-color: #ddeeff; }"
        "img { width: 24px; height: 18px; }"
        "</style></head><body><div>Loaded " +
        request.url + " <img src=x></div></body></html>");
  }
};

class RecordingCanvas final : public speed::platform::window::Canvas
{
public:
  void FillRect(speed::platform::window::Rect /*rect*/,
                speed::platform::window::Color /*color*/) override
  {
    ++fill_count;
  }

  void StrokeRect(speed::platform::window::Rect /*rect*/,
                  speed::platform::window::Color /*color*/,
                  int /*stroke_width*/) override
  {
    ++stroke_count;
  }

  void DrawLine(int /*x1*/,
                int /*y1*/,
                int /*x2*/,
                int /*y2*/,
                speed::platform::window::Color /*color*/,
                int /*stroke_width*/) override
  {
    ++line_count;
  }

  void DrawText(std::string_view text,
                int /*x*/,
                int /*y*/,
                int /*font_size_px*/,
                speed::platform::window::Color /*color*/) override
  {
    texts.push_back(std::string(text));
  }

  void PushClip(speed::platform::window::Rect /*rect*/) override
  {
    ++clip_depth;
  }

  void PopClip() override
  {
    --clip_depth;
  }

  int fill_count{0};
  int stroke_count{0};
  int line_count{0};
  int clip_depth{0};
  std::vector<std::string> texts;
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
      .fetch_adapter = std::make_unique<StaticFetchAdapter>(),
  });

  speed::base::Status status = app.Start();
  assert(status.ok());
  speed::ui::BrowserShellSnapshot snapshot = app.Snapshot();
  const speed::base::TabId first_tab = snapshot.active_tab;
  assert(first_tab);
  assert(snapshot.history.size() == 1);
  assert(snapshot.active_page.has_value());

  status = app.NavigateActiveTab("allowed.example/page");
  assert(status.ok());
  snapshot = app.Snapshot();
  assert(snapshot.history.size() == 2);
  assert(snapshot.history.back() == "https://allowed.example/page");
  assert(snapshot.active_page.has_value());
  assert(!snapshot.active_page->display_list.commands.empty());

  RecordingCanvas canvas;
  const speed::ui::gui::DisplayListRenderer renderer;
  renderer.Render(
      snapshot.active_page->display_list, canvas, {.x = 0, .y = 0, .width = 800, .height = 600}, 0);
  assert(canvas.fill_count > 0);
  assert(canvas.clip_depth == 0);

  speed::base::TabId second_tab;
  status = app.CreateTab(second_tab);
  assert(status.ok());
  assert(second_tab);
  snapshot = app.Snapshot();
  assert(snapshot.active_tab == second_tab);

  status = app.NavigateActiveTab("second.example/page");
  assert(status.ok());
  snapshot = app.Snapshot();
  assert(snapshot.history.size() == 3);
  assert(FindTab(snapshot, second_tab) != nullptr);

  status = app.SwitchToTab(first_tab);
  assert(status.ok());
  snapshot = app.Snapshot();
  assert(snapshot.active_tab == first_tab);

  status = app.CloseTab(second_tab);
  assert(status.ok());
  snapshot = app.Snapshot();
  assert(FindTab(snapshot, second_tab) == nullptr);

  status = app.NavigateActiveTab("https://ads.example/tracker");
  assert(!status.ok());
  snapshot = app.Snapshot();
  assert(snapshot.history.size() == 3);
  assert(snapshot.active_page.has_value());
  assert(snapshot.active_page->is_error_page);
  const speed::ui::ShellTabSnapshot* first_snapshot = FindTab(snapshot, first_tab);
  assert(first_snapshot != nullptr);
  assert(first_snapshot->navigation_state == speed::ui::ShellTabNavigationState::kBlocked);
  assert(first_snapshot->last_error == "blocked by embedded Aegis MVP rule");
  assert(!snapshot.active_page->display_list.commands.empty());

  status = app.Shutdown();
  assert(status.ok());
  return 0;
}
