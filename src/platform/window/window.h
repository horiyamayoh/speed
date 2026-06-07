#pragma once

#include "base/result/status.h"
#include "platform/window/canvas.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace speed::platform::window
{

enum class WindowEventType
{
  kCloseRequested,
  kExpose,
  kResize,
  kKeyPress,
  kTextInput,
  kMouseDown,
  kMouseWheel,
};

enum class Key
{
  kUnknown,
  kEnter,
  kBackspace,
  kEscape,
  kTab,
};

struct WindowEvent final
{
  WindowEventType type{WindowEventType::kExpose};
  Key key{Key::kUnknown};
  char text{0};
  int x{0};
  int y{0};
  int width{0};
  int height{0};
  int scroll_delta_y{0};
};

struct WindowConfig final
{
  std::string title{"Speed"};
  int width{1000};
  int height{720};
};

class PlatformWindow
{
public:
  virtual ~PlatformWindow() = default;

  [[nodiscard]] virtual base::Status Show() = 0;
  [[nodiscard]] virtual std::optional<WindowEvent> PollEvent() = 0;
  virtual void RequestRedraw() = 0;
  [[nodiscard]] virtual base::Status Draw(const std::function<void(Canvas&)>& draw_callback) = 0;
  [[nodiscard]] virtual int width() const = 0;
  [[nodiscard]] virtual int height() const = 0;
};

[[nodiscard]] std::unique_ptr<PlatformWindow> CreatePlatformWindow(WindowConfig config);

} // namespace speed::platform::window
