#include "browser/browser_process.h"

#include "base/logging/logging.h"
#include "browser/tabs/tab_model.h"
#include "storage/history/history_store.h"
#include "ui/shell/browser_shell.h"

namespace speed::browser
{

int RunBrowserProcess()
{
  base::Log(base::LogLevel::kInfo, "browser", "browser process stub starting");

  TabModel tabs;
  const base::TabId tab_id = tabs.CreateTab();
  (void)tab_id;

  storage::HistoryStore history;
  (void)history.RecordVisit("about:blank");

  const ui::BrowserShell shell;
  shell.Show();

  base::Log(base::LogLevel::kInfo, "browser", "browser process stub ready");
  return 0;
}

} // namespace speed::browser
