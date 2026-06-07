#include "app/browser_app.h"
#include "base/logging/logging.h"
#include "ui/shell/browser_shell.h"

#include <iostream>
#include <string_view>

namespace
{

[[nodiscard]] bool IsHelpArgument(std::string_view argument)
{
  return argument == "-h" || argument == "--help";
}

void PrintUsage(std::ostream& output)
{
  output << "usage: speed-browser [url]\n";
  output << "Run without a URL for the interactive console shell.\n";
}

} // namespace

int main(int argc, char* argv[])
{
  if (argc > 2)
  {
    PrintUsage(std::cerr);
    return 1;
  }

  if (argc == 2 && IsHelpArgument(argv[1]))
  {
    PrintUsage(std::cout);
    return 0;
  }

  speed::app::BrowserApp app;
  const speed::base::Status start_status = app.Start();
  if (!start_status.ok())
  {
    speed::base::Log(speed::base::LogLevel::kError, "browser", start_status.message());
    return 1;
  }

  speed::ui::BrowserShell shell(app, std::cin, std::cout);
  if (argc == 2)
  {
    const speed::base::Status smoke_status = shell.RunSmokeNavigation(argv[1]);
    return smoke_status.ok() ? 0 : 1;
  }

  return shell.Run();
}
