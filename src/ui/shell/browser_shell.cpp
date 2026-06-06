#include "ui/shell/browser_shell.h"

#include "base/logging/logging.h"

namespace speed::ui
{

void BrowserShell::Show() const
{
  base::Log(base::LogLevel::kInfo, "ui", "browser shell stub ready");
}

} // namespace speed::ui
