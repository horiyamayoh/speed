#include "platform/window/window.h"
#include "ui/gui/browser_gui_shell.h"

#include <cassert>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

class FakeWindow final : public speed::platform::window::PlatformWindow
{
public:
  explicit FakeWindow(std::deque<speed::platform::window::WindowEvent> events)
      : events_(std::move(events))
  {}

  speed::base::Status Show() override
  {
    return speed::base::Status::Ok();
  }

  std::optional<speed::platform::window::WindowEvent> PollEvent() override
  {
    if (events_.empty())
    {
      return std::nullopt;
    }

    speed::platform::window::WindowEvent event = events_.front();
    events_.pop_front();
    return event;
  }

  void RequestRedraw() override {}

  speed::base::Status
  Draw(const std::function<void(speed::platform::window::Canvas&)>& /*draw_callback*/) override
  {
    return speed::base::Status::Ok();
  }

  int width() const override
  {
    return 1000;
  }

  int height() const override
  {
    return 720;
  }

private:
  std::deque<speed::platform::window::WindowEvent> events_;
};

class FakeDelegate final : public speed::ui::BrowserShellDelegate
{
public:
  speed::base::Status CreateTab(speed::base::TabId& created_tab) override
  {
    created_tab = speed::base::TabId::FromRaw(2);
    return speed::base::Status::Ok();
  }

  speed::base::Status SwitchToTab(speed::base::TabId /*tab_id*/) override
  {
    return speed::base::Status::Ok();
  }

  speed::base::Status CloseTab(speed::base::TabId /*tab_id*/) override
  {
    return speed::base::Status::Ok();
  }

  speed::base::Status NavigateActiveTab(std::string_view input) override
  {
    navigations.push_back(std::string(input));
    return speed::base::Status::Ok();
  }

  speed::ui::BrowserShellSnapshot Snapshot() const override
  {
    return {
        .active_tab = speed::base::TabId::FromRaw(1),
        .tabs =
            {
                {
                    .id = speed::base::TabId::FromRaw(1),
                    .navigation_state = speed::ui::ShellTabNavigationState::kCommitted,
                    .pending_url = {},
                    .current_url = "about:blank",
                    .last_error = {},
                },
            },
        .history = {"https://history.example/page"},
        .active_page = std::nullopt,
    };
  }

  speed::base::Status Shutdown() override
  {
    shutdown_called = true;
    return speed::base::Status::Ok();
  }

  std::vector<std::string> navigations;
  bool shutdown_called{false};
};

} // namespace

int main()
{
  using speed::platform::window::WindowEvent;
  using speed::platform::window::WindowEventType;

  std::deque<WindowEvent> events;
  events.push_back({
      .type = WindowEventType::kMouseDown,
      .x = 950,
      .y = 50,
  });
  events.push_back({
      .type = WindowEventType::kMouseDown,
      .x = 20,
      .y = 112,
  });

  FakeDelegate delegate;
  auto window = std::make_unique<FakeWindow>(std::move(events));
  speed::ui::gui::BrowserGuiShell shell(delegate, std::move(window), {.smoke_exit_after_ms = 10});
  const int exit_code = shell.Run();
  assert(exit_code == 0);
  assert(delegate.navigations.size() == 1);
  assert(delegate.navigations.front() == "https://history.example/page");
  assert(delegate.shutdown_called);
  return 0;
}
