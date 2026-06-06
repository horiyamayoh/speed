#include "browser/browser_process.h"

#include <cassert>

int main()
{
  speed::browser::BrowserProcess process;
  assert(process.state() == speed::browser::BrowserProcess::State::kCreated);
  assert(!process.running());
  assert(process.tabs().tab_count() == 0);

  speed::base::Status status = process.NavigateActiveTab("example.test");
  assert(!status.ok());

  status = process.Start();
  assert(status.ok());
  assert(process.running());
  assert(process.tabs().tab_count() == 1);
  assert(process.tabs().active_tab());
  assert(process.history().entry_count() == 1);
  assert(process.history().entries().front() == "about:blank");

  status = process.Start();
  assert(!status.ok());

  status = process.NavigateActiveTab("Example.TEST/page");
  assert(status.ok());
  assert(process.history().entry_count() == 2);
  assert(process.history().entries().back() == "https://example.test/page");

  status = process.NavigateTab(speed::base::TabId::FromRaw(99), "example.test");
  assert(!status.ok());

  status = process.NavigateActiveTab("file:///tmp/speed");
  assert(!status.ok());
  assert(process.history().entry_count() == 2);

  status = process.Shutdown();
  assert(status.ok());
  assert(!process.running());
  assert(process.state() == speed::browser::BrowserProcess::State::kStopped);
  assert(process.tabs().tab_count() == 0);

  status = process.Shutdown();
  assert(!status.ok());

  return 0;
}
