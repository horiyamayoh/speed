#include "platform/window/window.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>

#ifdef Status
#undef Status
#endif

#include <algorithm>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace speed::platform::window
{

namespace
{

constexpr int kMinimumWindowWidth = 320;
constexpr int kMinimumWindowHeight = 240;

[[nodiscard]] bool HasArea(Rect rect)
{
  return rect.width > 0 && rect.height > 0;
}

[[nodiscard]] double ColorComponent(std::uint8_t value)
{
  return static_cast<double>(value) / 255.0;
}

void SetSourceColor(cairo_t* context, Color color)
{
  cairo_set_source_rgba(context,
                        ColorComponent(color.red),
                        ColorComponent(color.green),
                        ColorComponent(color.blue),
                        ColorComponent(color.alpha));
}

struct CairoSurfaceDeleter final
{
  void operator()(cairo_surface_t* surface) const
  {
    cairo_surface_destroy(surface);
  }
};

struct CairoContextDeleter final
{
  void operator()(cairo_t* context) const
  {
    cairo_destroy(context);
  }
};

using ScopedCairoSurface = std::unique_ptr<cairo_surface_t, CairoSurfaceDeleter>;
using ScopedCairoContext = std::unique_ptr<cairo_t, CairoContextDeleter>;

class CairoCanvas final : public Canvas
{
public:
  explicit CairoCanvas(cairo_t* context)
      : context_(context)
  {}

  void FillRect(Rect rect, Color color) override
  {
    if (!HasArea(rect))
    {
      return;
    }

    SetSourceColor(context_, color);
    cairo_rectangle(context_, rect.x, rect.y, rect.width, rect.height);
    cairo_fill(context_);
  }

  void StrokeRect(Rect rect, Color color, int stroke_width) override
  {
    if (!HasArea(rect) || stroke_width <= 0)
    {
      return;
    }

    SetSourceColor(context_, color);
    cairo_set_line_width(context_, stroke_width);
    const double offset = static_cast<double>(stroke_width) / 2.0;
    cairo_rectangle(context_,
                    static_cast<double>(rect.x) + offset,
                    static_cast<double>(rect.y) + offset,
                    std::max(0, rect.width - stroke_width),
                    std::max(0, rect.height - stroke_width));
    cairo_stroke(context_);
  }

  void DrawLine(int x1, int y1, int x2, int y2, Color color, int stroke_width) override
  {
    if (stroke_width <= 0)
    {
      return;
    }

    SetSourceColor(context_, color);
    cairo_set_line_width(context_, stroke_width);
    cairo_move_to(context_, x1, y1);
    cairo_line_to(context_, x2, y2);
    cairo_stroke(context_);
  }

  void DrawText(std::string_view text, int x, int y, int font_size_px, Color color) override
  {
    if (text.empty() || font_size_px <= 0)
    {
      return;
    }

    SetSourceColor(context_, color);
    cairo_select_font_face(
        context_, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(context_, font_size_px);
    cairo_move_to(context_, x, y + font_size_px);
    cairo_show_text(context_, std::string(text).c_str());
  }

  void PushClip(Rect rect) override
  {
    cairo_save(context_);
    if (!HasArea(rect))
    {
      cairo_rectangle(context_, 0, 0, 0, 0);
      cairo_clip(context_);
      return;
    }

    cairo_rectangle(context_, rect.x, rect.y, rect.width, rect.height);
    cairo_clip(context_);
  }

  void PopClip() override
  {
    cairo_restore(context_);
  }

private:
  cairo_t* context_{nullptr};
};

[[nodiscard]] Key MapKey(KeySym key_symbol)
{
  switch (key_symbol)
  {
  case XK_Return:
  case XK_KP_Enter:
    return Key::kEnter;
  case XK_BackSpace:
    return Key::kBackspace;
  case XK_Escape:
    return Key::kEscape;
  case XK_Tab:
    return Key::kTab;
  default:
    return Key::kUnknown;
  }
}

} // namespace

class X11PlatformWindow final : public PlatformWindow
{
public:
  explicit X11PlatformWindow(WindowConfig config)
      : config_(std::move(config)),
        width_(std::max(kMinimumWindowWidth, config_.width)),
        height_(std::max(kMinimumWindowHeight, config_.height))
  {}

  ~X11PlatformWindow() override
  {
    surface_.reset();
    if (display_ != nullptr && window_ != 0)
    {
      XDestroyWindow(display_, window_);
      window_ = 0;
    }

    if (display_ != nullptr)
    {
      XCloseDisplay(display_);
      display_ = nullptr;
    }
  }

  [[nodiscard]] base::Status Show() override
  {
    if (display_ != nullptr)
    {
      return base::Status::Ok();
    }

    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr)
    {
      return base::Status::Error("failed to open X11 display");
    }

    screen_ = DefaultScreen(display_);
    window_ = XCreateSimpleWindow(display_,
                                  RootWindow(display_, screen_),
                                  0,
                                  0,
                                  static_cast<unsigned int>(width_),
                                  static_cast<unsigned int>(height_),
                                  0,
                                  BlackPixel(display_, screen_),
                                  WhitePixel(display_, screen_));
    if (window_ == 0)
    {
      return base::Status::Error("failed to create X11 window");
    }

    XStoreName(display_, window_, config_.title.c_str());
    XSelectInput(
        display_, window_, ExposureMask | StructureNotifyMask | KeyPressMask | ButtonPressMask);
    wm_delete_window_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display_, window_, &wm_delete_window_, 1);
    XMapWindow(display_, window_);
    XFlush(display_);
    return base::Status::Ok();
  }

  [[nodiscard]] std::optional<WindowEvent> PollEvent() override
  {
    if (!pending_events_.empty())
    {
      WindowEvent event = pending_events_.front();
      pending_events_.pop_front();
      return event;
    }

    if (display_ == nullptr || XPending(display_) == 0)
    {
      return std::nullopt;
    }

    XEvent x_event{};
    XNextEvent(display_, &x_event);
    QueueEvent(x_event);

    if (pending_events_.empty())
    {
      return std::nullopt;
    }

    WindowEvent event = pending_events_.front();
    pending_events_.pop_front();
    return event;
  }

  void RequestRedraw() override
  {
    if (display_ == nullptr || window_ == 0)
    {
      return;
    }

    XClearArea(display_, window_, 0, 0, 0, 0, True);
    XFlush(display_);
  }

  [[nodiscard]] base::Status Draw(const std::function<void(Canvas&)>& draw_callback) override
  {
    if (display_ == nullptr || window_ == 0)
    {
      return base::Status::Error("window is not visible");
    }

    EnsureSurface();
    if (!surface_)
    {
      return base::Status::Error("failed to create Cairo X11 surface");
    }

    ScopedCairoContext context(cairo_create(surface_.get()));
    if (!context)
    {
      return base::Status::Error("failed to create Cairo context");
    }

    CairoCanvas canvas(context.get());
    draw_callback(canvas);
    cairo_surface_flush(surface_.get());
    XFlush(display_);
    return base::Status::Ok();
  }

  [[nodiscard]] int width() const override
  {
    return width_;
  }

  [[nodiscard]] int height() const override
  {
    return height_;
  }

private:
  void QueueEvent(const XEvent& x_event)
  {
    switch (x_event.type)
    {
    case Expose:
      pending_events_.push_back({.type = WindowEventType::kExpose});
      return;
    case ConfigureNotify:
      width_ = std::max(kMinimumWindowWidth, x_event.xconfigure.width);
      height_ = std::max(kMinimumWindowHeight, x_event.xconfigure.height);
      if (surface_)
      {
        cairo_xlib_surface_set_size(surface_.get(), width_, height_);
      }
      pending_events_.push_back({
          .type = WindowEventType::kResize,
          .width = width_,
          .height = height_,
      });
      return;
    case ClientMessage:
      if (static_cast<Atom>(x_event.xclient.data.l[0]) == wm_delete_window_)
      {
        pending_events_.push_back({.type = WindowEventType::kCloseRequested});
      }
      return;
    case ButtonPress:
      QueueButtonEvent(x_event.xbutton);
      return;
    case KeyPress:
      QueueKeyEvent(x_event.xkey);
      return;
    default:
      return;
    }
  }

  void QueueButtonEvent(const XButtonEvent& button)
  {
    if (button.button == Button4 || button.button == Button5)
    {
      pending_events_.push_back({
          .type = WindowEventType::kMouseWheel,
          .x = button.x,
          .y = button.y,
          .scroll_delta_y = button.button == Button4 ? -64 : 64,
      });
      return;
    }

    if (button.button != Button1)
    {
      return;
    }

    if (display_ != nullptr && window_ != 0)
    {
      XSetInputFocus(display_, window_, RevertToParent, CurrentTime);
      XFlush(display_);
    }

    pending_events_.push_back({
        .type = WindowEventType::kMouseDown,
        .x = button.x,
        .y = button.y,
    });
  }

  void QueueKeyEvent(const XKeyEvent& key)
  {
    KeySym key_symbol = NoSymbol;
    char buffer[32]{};
    XKeyEvent key_copy = key;
    const int byte_count = XLookupString(&key_copy, buffer, sizeof(buffer), &key_symbol, nullptr);
    pending_events_.push_back({
        .type = WindowEventType::kKeyPress,
        .key = MapKey(key_symbol),
    });

    for (int index = 0; index < byte_count; ++index)
    {
      const unsigned char byte = static_cast<unsigned char>(buffer[index]);
      if (byte >= 32U && byte <= 126U)
      {
        pending_events_.push_back({
            .type = WindowEventType::kTextInput,
            .text = static_cast<char>(byte),
        });
      }
    }
  }

  void EnsureSurface()
  {
    if (surface_)
    {
      return;
    }

    Visual* const visual = DefaultVisual(display_, screen_);
    surface_.reset(cairo_xlib_surface_create(display_, window_, visual, width_, height_));
  }

  WindowConfig config_;
  Display* display_{nullptr};
  int screen_{0};
  Window window_{0};
  Atom wm_delete_window_{0};
  int width_{1000};
  int height_{720};
  ScopedCairoSurface surface_;
  std::deque<WindowEvent> pending_events_;
};

std::unique_ptr<PlatformWindow> CreatePlatformWindow(WindowConfig config)
{
  return std::make_unique<X11PlatformWindow>(std::move(config));
}

} // namespace speed::platform::window
