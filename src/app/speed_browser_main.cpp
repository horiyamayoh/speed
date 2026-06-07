#include "app/browser_app.h"
#include "base/logging/logging.h"
#include "ui/shell/browser_shell.h"

#include <iostream>
#include <string_view>
#include <utility>

namespace
{

[[nodiscard]] bool IsHelpArgument(std::string_view argument)
{
  return argument == "-h" || argument == "--help";
}

void PrintUsage(std::ostream& output)
{
  output << "usage: speed-browser [--process-model=in-process|multi-process] [url]\n";
  output << "Run without a URL for the interactive console shell.\n";
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
  speed::app::BrowserAppOptions options;
  std::string_view smoke_url;
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

    if (argument.starts_with("--") || !smoke_url.empty())
    {
      PrintUsage(std::cerr);
      return 1;
    }

    smoke_url = argument;
  }

  speed::app::BrowserApp app(std::move(options));
  const speed::base::Status start_status = app.Start();
  if (!start_status.ok())
  {
    speed::base::Log(speed::base::LogLevel::kError, "browser", start_status.message());
    return 1;
  }

  speed::ui::BrowserShell shell(app, std::cin, std::cout);
  if (!smoke_url.empty())
  {
    const speed::base::Status smoke_status = shell.RunSmokeNavigation(smoke_url);
    return smoke_status.ok() ? 0 : 1;
  }

  return shell.Run();
}
