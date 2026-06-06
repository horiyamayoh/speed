#include "browser/tabs/tab_model.h"

#include <cassert>

int main()
{
  speed::browser::TabModel tabs;
  assert(tabs.tab_count() == 0);
  assert(!tabs.active_tab());

  const speed::base::TabId first = tabs.CreateTab();
  const speed::base::TabId second = tabs.CreateTab();
  const speed::base::TabId third = tabs.CreateTab();

  assert(tabs.tab_count() == 3);
  assert(first.value() == 1);
  assert(second.value() == 2);
  assert(third.value() == 3);
  assert(tabs.active_tab() == third);
  assert(tabs.ContainsTab(second));

  assert(tabs.SwitchToTab(first));
  assert(tabs.active_tab() == first);
  assert(!tabs.SwitchToTab(speed::base::TabId::FromRaw(99)));
  assert(tabs.active_tab() == first);

  assert(tabs.CloseTab(second));
  assert(tabs.tab_count() == 2);
  assert(tabs.active_tab() == first);
  assert(!tabs.ContainsTab(second));

  assert(tabs.CloseTab(first));
  assert(tabs.active_tab() == third);

  assert(tabs.CloseTab(third));
  assert(tabs.tab_count() == 0);
  assert(!tabs.active_tab());
  assert(!tabs.CloseTab(third));

  const speed::base::TabId replacement = tabs.CreateTab();
  assert(replacement.value() == 4);
  tabs.CloseAllTabs();
  assert(tabs.tab_count() == 0);
  assert(!tabs.active_tab());

  return 0;
}
