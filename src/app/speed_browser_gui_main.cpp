#include "app/browser_app.h"
#include "base/logging/logging.h"
#include "platform/window/window.h"
#include "ui/gui/browser_gui_shell.h"

#include <charconv>
#include <iostream>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{

[[nodiscard]] bool IsHelpArgument(std::string_view argument)
{
  return argument == "-h" || argument == "--help";
}

[[nodiscard]] bool ParseSmokeExit(std::string_view argument, int& milliseconds)
{
  constexpr std::string_view prefix = "--smoke-exit-after-ms=";
  if (!argument.starts_with(prefix))
  {
    return false;
  }

  std::string_view value = argument.substr(prefix.size());
  int parsed = 0;
  const char* const begin = value.data();
  const char* const end = value.data() + value.size();
  const std::from_chars_result result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end || parsed <= 0)
  {
    return false;
  }

  milliseconds = parsed;
  return true;
}

void PrintUsage(std::ostream& output)
{
  output << "usage: speed-browser-gui [--process-model=in-process|multi-process] "
            "[--smoke-exit-after-ms=<ms>]\n";
  output << "Starts the minimal Speed GUI BrowserShell.\n";
}

[[nodiscard]] bool ParseProcessModel(std::string_view argument,
                                     speed::app::BrowserAppProcessModel& process_model)
{
  constexpr std::string_view prefix = "--process-model=";
  if (!argument.starts_with(prefix))
  {
    return false;
  }

  const std::string_view value = argument.substr(prefix.size());
  if (value == "in-process")
  {
    process_model = speed::app::BrowserAppProcessModel::kInProcess;
    return true;
  }

  if (value == "multi-process")
  {
    process_model = speed::app::BrowserAppProcessModel::kMultiProcess;
    return true;
  }

  return false;
}

} // namespace

int main(int argc, char* argv[])
{
  int smoke_exit_after_ms = 0;
  speed::app::BrowserAppOptions options;
  for (int index = 1; index < argc; ++index)
  {
    const std::string_view argument(argv[index]);
    if (IsHelpArgument(argument))
    {
      PrintUsage(std::cout);
      return 0;
    }

    if (ParseProcessModel(argument, options.process_model))
    {
      continue;
    }

    if (ParseSmokeExit(argument, smoke_exit_after_ms))
    {
      continue;
    }

    PrintUsage(std::cerr);
    return 1;
  }

  speed::app::BrowserApp app(std::move(options));
  const speed::base::Status start_status = app.Start();
  if (!start_status.ok())
  {
    speed::base::Log(speed::base::LogLevel::kError, "browser", start_status.message());
    return 1;
  }

  std::unique_ptr<speed::platform::window::PlatformWindow> window =
      speed::platform::window::CreatePlatformWindow({
          .title = "Speed v0.1",
          .width = 1000,
          .height = 720,
      });
  speed::ui::gui::BrowserGuiShell gui_shell(
      app, std::move(window), {.smoke_exit_after_ms = smoke_exit_after_ms});
  return gui_shell.Run();
}
