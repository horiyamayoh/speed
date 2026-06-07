#pragma once

#include "platform/window/window.h"
#include "ui/gui/address_bar_model.h"
#include "ui/gui/display_list_renderer.h"
#include "ui/shell/browser_shell.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace speed::ui::gui
{

struct BrowserGuiShellOptions final
{
  int smoke_exit_after_ms{0};
};

class BrowserGuiShell final
{
public:
  BrowserGuiShell(BrowserShellDelegate& delegate,
                  std::unique_ptr<platform::window::PlatformWindow> window,
                  BrowserGuiShellOptions options = {});

  [[nodiscard]] int Run();

private:
  struct TabHitTarget final
  {
    base::TabId tab_id;
    platform::window::Rect tab_rect;
    platform::window::Rect close_rect;
  };

  struct Layout final
  {
    platform::window::Rect new_tab_rect;
    platform::window::Rect address_rect;
    platform::window::Rect go_rect;
    platform::window::Rect history_rect;
    platform::window::Rect status_rect;
    platform::window::Rect history_panel_rect;
    platform::window::Rect page_rect;
    std::vector<TabHitTarget> tabs;
  };

  [[nodiscard]] Layout ComputeLayout(const BrowserShellSnapshot& snapshot) const;
  void ProcessEvent(const platform::window::WindowEvent& event);
  void ProcessMouseDown(int x, int y);
  void ProcessKeyPress(platform::window::Key key);
  void ProcessTextInput(char text);
  void NavigateAddressBar();
  void Draw(platform::window::Canvas& canvas);
  void DrawChrome(platform::window::Canvas& canvas,
                  const BrowserShellSnapshot& snapshot,
                  const Layout& layout) const;
  void DrawHistoryPanel(platform::window::Canvas& canvas,
                        const BrowserShellSnapshot& snapshot,
                        const Layout& layout) const;
  void DrawPage(platform::window::Canvas& canvas,
                const BrowserShellSnapshot& snapshot,
                const Layout& layout) const;
  void SyncAddressBarFromActiveTab();
  void ClampScroll();
  [[nodiscard]] std::string ActiveAddressText(const BrowserShellSnapshot& snapshot) const;
  [[nodiscard]] std::string StatusText(const BrowserShellSnapshot& snapshot) const;

  BrowserShellDelegate& delegate_;
  std::unique_ptr<platform::window::PlatformWindow> window_;
  DisplayListRenderer display_list_renderer_;
  BrowserGuiShellOptions options_;
  AddressBarModel address_bar_;
  std::string status_text_;
  bool address_focused_{false};
  bool history_visible_{false};
  int page_scroll_y_{0};
};

} // namespace speed::ui::gui
