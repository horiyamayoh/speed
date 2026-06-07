#include "ui/gui/browser_gui_shell.h"

#include "base/logging/logging.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <thread>

namespace speed::ui::gui
{

namespace
{

using platform::window::Canvas;
using platform::window::Color;
using platform::window::Rect;

constexpr int kTabStripHeight = 36;
constexpr int kToolbarHeight = 42;
constexpr int kStatusHeight = 24;
constexpr int kChromePadding = 8;
constexpr int kButtonSize = 26;
constexpr int kTabWidth = 152;
constexpr int kHistoryPanelWidth = 280;
constexpr int kHistoryRowHeight = 24;
constexpr int kPageBackgroundInset = 1;

constexpr Color kWindowBackground{.red = 238, .green = 240, .blue = 243, .alpha = 255};
constexpr Color kChromeBackground{.red = 225, .green = 229, .blue = 234, .alpha = 255};
constexpr Color kPanelBackground{.red = 248, .green = 249, .blue = 250, .alpha = 255};
constexpr Color kPageBackground{.red = 255, .green = 255, .blue = 255, .alpha = 255};
constexpr Color kBorder{.red = 156, .green = 163, .blue = 175, .alpha = 255};
constexpr Color kText{.red = 21, .green = 26, .blue = 33, .alpha = 255};
constexpr Color kMutedText{.red = 87, .green = 96, .blue = 108, .alpha = 255};
constexpr Color kAccent{.red = 32, .green = 107, .blue = 196, .alpha = 255};
constexpr Color kError{.red = 171, .green = 42, .blue = 42, .alpha = 255};

[[nodiscard]] bool Contains(Rect rect, int x, int y)
{
  return x >= rect.x && y >= rect.y && x < rect.x + rect.width && y < rect.y + rect.height;
}

[[nodiscard]] std::string Truncate(std::string_view text, std::size_t max_size)
{
  if (text.size() <= max_size)
  {
    return std::string(text);
  }

  if (max_size <= 3)
  {
    return std::string(text.substr(0, max_size));
  }

  std::string truncated(text.substr(0, max_size - 3));
  truncated += "...";
  return truncated;
}

[[nodiscard]] std::string TabStateName(ShellTabNavigationState state)
{
  switch (state)
  {
  case ShellTabNavigationState::kEmpty:
    return "empty";
  case ShellTabNavigationState::kLoading:
    return "loading";
  case ShellTabNavigationState::kCommitted:
    return "ready";
  case ShellTabNavigationState::kBlocked:
    return "blocked";
  case ShellTabNavigationState::kFailed:
    return "failed";
  case ShellTabNavigationState::kCrashed:
    return "crashed";
  }

  return "unknown";
}

[[nodiscard]] std::string DisplayUrl(const ShellTabSnapshot& tab)
{
  if (!tab.current_url.empty())
  {
    return tab.current_url;
  }

  if (!tab.pending_url.empty())
  {
    return tab.pending_url;
  }

  return "about:blank";
}

[[nodiscard]] const ShellTabSnapshot* FindActiveTab(const BrowserShellSnapshot& snapshot)
{
  if (!snapshot.active_tab)
  {
    return nullptr;
  }

  for (const ShellTabSnapshot& tab : snapshot.tabs)
  {
    if (tab.id == snapshot.active_tab)
    {
      return &tab;
    }
  }

  return nullptr;
}

void DrawButton(Canvas& canvas, Rect rect, std::string_view label, bool active = false)
{
  canvas.FillRect(rect,
                  active ? Color{.red = 216, .green = 232, .blue = 255, .alpha = 255}
                         : Color{.red = 248, .green = 250, .blue = 252, .alpha = 255});
  canvas.StrokeRect(rect, active ? kAccent : kBorder, 1);
  canvas.DrawText(label, rect.x + 7, rect.y + 4, 14, active ? kAccent : kText);
}

} // namespace

BrowserGuiShell::BrowserGuiShell(BrowserShellDelegate& delegate,
                                 std::unique_ptr<platform::window::PlatformWindow> window,
                                 BrowserGuiShellOptions options)
    : delegate_(delegate),
      window_(std::move(window)),
      options_(options)
{}

int BrowserGuiShell::Run()
{
  if (!window_)
  {
    base::Log(base::LogLevel::kError, "ui", "GUI shell window is not available");
    return 1;
  }

  const base::Status show_status = window_->Show();
  if (!show_status.ok())
  {
    base::Log(base::LogLevel::kError, "ui", show_status.message());
    return 1;
  }

  SyncAddressBarFromActiveTab();
  bool running = true;
  bool needs_redraw = true;
  const auto start_time = std::chrono::steady_clock::now();

  while (running)
  {
    while (const std::optional<platform::window::WindowEvent> event = window_->PollEvent())
    {
      if (event->type == platform::window::WindowEventType::kCloseRequested)
      {
        const base::Status shutdown_status = delegate_.Shutdown();
        if (!shutdown_status.ok())
        {
          base::Log(base::LogLevel::kError, "ui", shutdown_status.message());
        }
        running = false;
        break;
      }

      ProcessEvent(*event);
      needs_redraw = true;
    }

    if (needs_redraw)
    {
      const base::Status draw_status = window_->Draw([this](Canvas& canvas) { Draw(canvas); });
      if (!draw_status.ok())
      {
        base::Log(base::LogLevel::kError, "ui", draw_status.message());
        const base::Status shutdown_status = delegate_.Shutdown();
        if (!shutdown_status.ok())
        {
          base::Log(base::LogLevel::kError, "ui", shutdown_status.message());
        }
        return 1;
      }
      needs_redraw = false;
    }

    if (options_.smoke_exit_after_ms > 0)
    {
      const auto elapsed = std::chrono::steady_clock::now() - start_time;
      const auto elapsed_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
      if (elapsed_ms >= options_.smoke_exit_after_ms)
      {
        const base::Status shutdown_status = delegate_.Shutdown();
        if (!shutdown_status.ok())
        {
          base::Log(base::LogLevel::kError, "ui", shutdown_status.message());
          return 1;
        }
        return 0;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  return 0;
}

BrowserGuiShell::Layout BrowserGuiShell::ComputeLayout(const BrowserShellSnapshot& snapshot) const
{
  const int width = window_->width();
  const int height = window_->height();
  const int history_width = history_visible_ ? std::min(kHistoryPanelWidth, width / 2) : 0;

  Layout layout{
      .new_tab_rect =
          {
              .x = kChromePadding,
              .y = 5,
              .width = kButtonSize,
              .height = kButtonSize,
          },
      .address_rect =
          {
              .x = kChromePadding,
              .y = kTabStripHeight + 7,
              .width = std::max(120, width - 128),
              .height = 28,
          },
      .go_rect =
          {
              .x = std::max(132, width - 112),
              .y = kTabStripHeight + 7,
              .width = 40,
              .height = 28,
          },
      .history_rect =
          {
              .x = std::max(176, width - 64),
              .y = kTabStripHeight + 7,
              .width = 56,
              .height = 28,
          },
      .status_rect =
          {
              .x = 0,
              .y = height - kStatusHeight,
              .width = width,
              .height = kStatusHeight,
          },
      .history_panel_rect =
          {
              .x = 0,
              .y = kTabStripHeight + kToolbarHeight,
              .width = history_width,
              .height = std::max(0, height - kTabStripHeight - kToolbarHeight - kStatusHeight),
          },
      .page_rect =
          {
              .x = history_width,
              .y = kTabStripHeight + kToolbarHeight,
              .width = std::max(0, width - history_width),
              .height = std::max(0, height - kTabStripHeight - kToolbarHeight - kStatusHeight),
          },
      .tabs = {},
  };
  layout.address_rect.width = std::max(80, layout.go_rect.x - layout.address_rect.x - 8);

  int tab_x = layout.new_tab_rect.x + layout.new_tab_rect.width + 8;
  for (const ShellTabSnapshot& tab : snapshot.tabs)
  {
    if (tab_x + 56 > width)
    {
      break;
    }

    const int tab_width = std::min(kTabWidth, std::max(56, width - tab_x - kChromePadding));
    Rect tab_rect{
        .x = tab_x,
        .y = 5,
        .width = tab_width,
        .height = kButtonSize,
    };
    layout.tabs.push_back({
        .tab_id = tab.id,
        .tab_rect = tab_rect,
        .close_rect =
            {
                .x = tab_rect.x + tab_rect.width - 24,
                .y = tab_rect.y + 4,
                .width = 18,
                .height = 18,
            },
    });
    tab_x += tab_width + 4;
  }

  return layout;
}

void BrowserGuiShell::ProcessEvent(const platform::window::WindowEvent& event)
{
  switch (event.type)
  {
  case platform::window::WindowEventType::kExpose:
    return;
  case platform::window::WindowEventType::kResize:
    ClampScroll();
    return;
  case platform::window::WindowEventType::kKeyPress:
    ProcessKeyPress(event.key);
    return;
  case platform::window::WindowEventType::kTextInput:
    ProcessTextInput(event.text);
    return;
  case platform::window::WindowEventType::kMouseDown:
    ProcessMouseDown(event.x, event.y);
    return;
  case platform::window::WindowEventType::kMouseWheel:
    page_scroll_y_ = std::max(0, page_scroll_y_ + event.scroll_delta_y);
    ClampScroll();
    return;
  case platform::window::WindowEventType::kCloseRequested:
    return;
  }
}

void BrowserGuiShell::ProcessMouseDown(int x, int y)
{
  const BrowserShellSnapshot snapshot = delegate_.Snapshot();
  const Layout layout = ComputeLayout(snapshot);

  if (Contains(layout.new_tab_rect, x, y))
  {
    base::TabId created_tab;
    const base::Status status = delegate_.CreateTab(created_tab);
    status_text_ =
        status.ok() ? "created tab " + std::to_string(created_tab.value()) : status.message();
    SyncAddressBarFromActiveTab();
    page_scroll_y_ = 0;
    return;
  }

  for (const TabHitTarget& target : layout.tabs)
  {
    if (Contains(target.close_rect, x, y))
    {
      const base::Status status = delegate_.CloseTab(target.tab_id);
      status_text_ = status.ok() ? "closed tab" : status.message();
      SyncAddressBarFromActiveTab();
      page_scroll_y_ = 0;
      return;
    }

    if (Contains(target.tab_rect, x, y))
    {
      const base::Status status = delegate_.SwitchToTab(target.tab_id);
      status_text_ = status.ok() ? "switched tab" : status.message();
      SyncAddressBarFromActiveTab();
      page_scroll_y_ = 0;
      return;
    }
  }

  if (Contains(layout.address_rect, x, y))
  {
    address_focused_ = true;
    address_bar_.SelectAll();
    return;
  }

  address_focused_ = false;
  address_bar_.ClearSelection();

  if (Contains(layout.go_rect, x, y))
  {
    NavigateAddressBar();
    return;
  }

  if (Contains(layout.history_rect, x, y))
  {
    history_visible_ = !history_visible_;
    ClampScroll();
    return;
  }

  if (history_visible_ && Contains(layout.history_panel_rect, x, y))
  {
    const int row = (y - layout.history_panel_rect.y - 30) / kHistoryRowHeight;
    if (row >= 0 && static_cast<std::size_t>(row) < snapshot.history.size())
    {
      address_bar_.SetText(snapshot.history[static_cast<std::size_t>(row)]);
      NavigateAddressBar();
    }
  }
}

void BrowserGuiShell::ProcessKeyPress(platform::window::Key key)
{
  if (!address_focused_)
  {
    return;
  }

  switch (key)
  {
  case platform::window::Key::kEnter:
    NavigateAddressBar();
    return;
  case platform::window::Key::kBackspace:
    address_bar_.Backspace();
    return;
  case platform::window::Key::kEscape:
    address_focused_ = false;
    address_bar_.ClearSelection();
    SyncAddressBarFromActiveTab();
    return;
  case platform::window::Key::kTab:
  case platform::window::Key::kUnknown:
    return;
  }
}

void BrowserGuiShell::ProcessTextInput(char text)
{
  if (!address_focused_)
  {
    return;
  }

  if (text >= 32 && text <= 126)
  {
    address_bar_.InsertChar(text);
  }
}

void BrowserGuiShell::NavigateAddressBar()
{
  if (address_bar_.text().empty())
  {
    status_text_ = "enter a URL";
    return;
  }

  const base::Status status = delegate_.NavigateActiveTab(address_bar_.text());
  status_text_ = status.ok() ? "loaded " + address_bar_.text() : status.message();
  SyncAddressBarFromActiveTab();
  page_scroll_y_ = 0;
  ClampScroll();
}

void BrowserGuiShell::Draw(Canvas& canvas)
{
  const BrowserShellSnapshot snapshot = delegate_.Snapshot();
  const Layout layout = ComputeLayout(snapshot);
  canvas.FillRect({.x = 0, .y = 0, .width = window_->width(), .height = window_->height()},
                  kWindowBackground);
  DrawChrome(canvas, snapshot, layout);
  if (history_visible_)
  {
    DrawHistoryPanel(canvas, snapshot, layout);
  }
  DrawPage(canvas, snapshot, layout);
}

void BrowserGuiShell::DrawChrome(Canvas& canvas,
                                 const BrowserShellSnapshot& snapshot,
                                 const Layout& layout) const
{
  canvas.FillRect({.x = 0, .y = 0, .width = window_->width(), .height = kTabStripHeight},
                  kChromeBackground);
  canvas.FillRect(
      {
          .x = 0,
          .y = kTabStripHeight,
          .width = window_->width(),
          .height = kToolbarHeight,
      },
      kChromeBackground);
  canvas.DrawLine(0, kTabStripHeight - 1, window_->width(), kTabStripHeight - 1, kBorder, 1);
  canvas.DrawLine(0,
                  kTabStripHeight + kToolbarHeight - 1,
                  window_->width(),
                  kTabStripHeight + kToolbarHeight - 1,
                  kBorder,
                  1);

  DrawButton(canvas, layout.new_tab_rect, "+");

  for (const TabHitTarget& target : layout.tabs)
  {
    const auto tab = std::ranges::find(snapshot.tabs, target.tab_id, &ShellTabSnapshot::id);
    if (tab == snapshot.tabs.end())
    {
      continue;
    }

    const bool active = target.tab_id == snapshot.active_tab;
    canvas.FillRect(target.tab_rect,
                    active ? Color{.red = 255, .green = 255, .blue = 255, .alpha = 255}
                           : Color{.red = 238, .green = 241, .blue = 245, .alpha = 255});
    canvas.StrokeRect(target.tab_rect, active ? kAccent : kBorder, 1);
    const std::string label =
        Truncate(std::to_string(tab->id.value()) + " " + DisplayUrl(*tab),
                 static_cast<std::size_t>(std::max(4, target.tab_rect.width / 9 - 4)));
    canvas.DrawText(label, target.tab_rect.x + 8, target.tab_rect.y + 5, 13, kText);
    canvas.DrawText("x", target.close_rect.x + 5, target.close_rect.y + 1, 13, kMutedText);
  }

  canvas.FillRect(layout.address_rect, kPageBackground);
  canvas.StrokeRect(layout.address_rect, address_focused_ ? kAccent : kBorder, 1);
  const std::string address = Truncate(
      address_bar_.text(), static_cast<std::size_t>(std::max(8, layout.address_rect.width / 8)));
  if (address_focused_ && address_bar_.has_selection() && !address.empty())
  {
    const std::size_t selected_count =
        std::min(address.size(), address_bar_.selection_end() - address_bar_.selection_start());
    const Rect selection_rect{
        .x = layout.address_rect.x + 8 + static_cast<int>(address_bar_.selection_start()) * 8,
        .y = layout.address_rect.y + 4,
        .width = std::min(layout.address_rect.width - 8, static_cast<int>(selected_count) * 8 + 4),
        .height = layout.address_rect.height - 8,
    };
    canvas.FillRect(selection_rect, kAccent);
    canvas.DrawText(
        address, layout.address_rect.x + 8, layout.address_rect.y + 5, 14, kPageBackground);
  }
  else
  {
    canvas.DrawText(address, layout.address_rect.x + 8, layout.address_rect.y + 5, 14, kText);
    if (address_focused_)
    {
      const int caret_x = layout.address_rect.x + 8 +
                          (static_cast<int>(std::min(address_bar_.caret(), address.size())) * 8);
      canvas.DrawLine(caret_x,
                      layout.address_rect.y + 5,
                      caret_x,
                      layout.address_rect.y + layout.address_rect.height - 5,
                      kAccent,
                      1);
    }
  }
  DrawButton(canvas, layout.go_rect, "Go");
  DrawButton(canvas, layout.history_rect, "Hist", history_visible_);

  canvas.FillRect(layout.status_rect, Color{.red = 247, .green = 248, .blue = 250, .alpha = 255});
  canvas.DrawLine(layout.status_rect.x,
                  layout.status_rect.y,
                  layout.status_rect.x + layout.status_rect.width,
                  layout.status_rect.y,
                  kBorder,
                  1);
  const std::string status =
      Truncate(StatusText(snapshot), static_cast<std::size_t>(std::max(8, window_->width() / 8)));
  const ShellTabSnapshot* const active_tab = FindActiveTab(snapshot);
  const bool is_error =
      active_tab != nullptr && (active_tab->navigation_state == ShellTabNavigationState::kBlocked ||
                                active_tab->navigation_state == ShellTabNavigationState::kFailed ||
                                active_tab->navigation_state == ShellTabNavigationState::kCrashed);
  canvas.DrawText(status,
                  layout.status_rect.x + 8,
                  layout.status_rect.y + 4,
                  13,
                  is_error ? kError : kMutedText);
}

void BrowserGuiShell::DrawHistoryPanel(Canvas& canvas,
                                       const BrowserShellSnapshot& snapshot,
                                       const Layout& layout) const
{
  canvas.FillRect(layout.history_panel_rect, kPanelBackground);
  canvas.StrokeRect(layout.history_panel_rect, kBorder, 1);
  canvas.DrawText(
      "History", layout.history_panel_rect.x + 10, layout.history_panel_rect.y + 8, 14, kText);

  int y = layout.history_panel_rect.y + 32;
  for (const std::string& entry : snapshot.history)
  {
    if (y + kHistoryRowHeight > layout.history_panel_rect.y + layout.history_panel_rect.height)
    {
      break;
    }

    canvas.DrawText(Truncate(entry, 31), layout.history_panel_rect.x + 10, y + 3, 12, kMutedText);
    y += kHistoryRowHeight;
  }
}

void BrowserGuiShell::DrawPage(Canvas& canvas,
                               const BrowserShellSnapshot& snapshot,
                               const Layout& layout) const
{
  const Rect page_rect{
      .x = layout.page_rect.x + kPageBackgroundInset,
      .y = layout.page_rect.y + kPageBackgroundInset,
      .width = std::max(0, layout.page_rect.width - (kPageBackgroundInset * 2)),
      .height = std::max(0, layout.page_rect.height - (kPageBackgroundInset * 2)),
  };
  canvas.FillRect(page_rect, kPageBackground);
  canvas.StrokeRect(layout.page_rect, kBorder, 1);

  if (!snapshot.active_page.has_value())
  {
    canvas.DrawText("No committed document", page_rect.x + 20, page_rect.y + 20, 16, kMutedText);
    return;
  }

  display_list_renderer_.Render(
      snapshot.active_page->display_list, canvas, page_rect, page_scroll_y_);
}

void BrowserGuiShell::SyncAddressBarFromActiveTab()
{
  const BrowserShellSnapshot snapshot = delegate_.Snapshot();
  address_bar_.SetText(ActiveAddressText(snapshot));
}

void BrowserGuiShell::ClampScroll()
{
  const BrowserShellSnapshot snapshot = delegate_.Snapshot();
  const Layout layout = ComputeLayout(snapshot);
  int content_height = 0;
  if (snapshot.active_page.has_value())
  {
    content_height = snapshot.active_page->content_height;
  }

  const int max_scroll = std::max(0, content_height - layout.page_rect.height);
  page_scroll_y_ = std::clamp(page_scroll_y_, 0, max_scroll);
}

std::string BrowserGuiShell::ActiveAddressText(const BrowserShellSnapshot& snapshot) const
{
  const ShellTabSnapshot* const active_tab = FindActiveTab(snapshot);
  if (active_tab == nullptr)
  {
    return {};
  }

  return DisplayUrl(*active_tab);
}

std::string BrowserGuiShell::StatusText(const BrowserShellSnapshot& snapshot) const
{
  if (!status_text_.empty())
  {
    return status_text_;
  }

  const ShellTabSnapshot* const active_tab = FindActiveTab(snapshot);
  if (active_tab == nullptr)
  {
    return "no active tab";
  }

  if (!active_tab->last_error.empty())
  {
    return active_tab->last_error;
  }

  return TabStateName(active_tab->navigation_state) + " " + DisplayUrl(*active_tab);
}

} // namespace speed::ui::gui
