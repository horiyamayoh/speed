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
  const speed::browser::TabState* first_state = tabs.GetTabState(first);
  const speed::browser::TabState* second_state = tabs.GetTabState(second);
  assert(first_state != nullptr);
  assert(second_state != nullptr);
  assert(first_state->navigation_state == speed::browser::TabNavigationState::kEmpty);

  assert(tabs.StartNavigation(first,
                              speed::base::RequestId::FromRaw(11),
                              "https://first.test")
             .ok());
  assert(tabs.CommitNavigation(first,
                               speed::base::RequestId::FromRaw(11),
                               speed::base::DocumentId::FromRaw(21),
                               "https://first.test")
             .ok());
  assert(tabs.StartNavigation(second,
                              speed::base::RequestId::FromRaw(12),
                              "https://blocked.test")
             .ok());
  assert(tabs.BlockNavigation(second,
                              speed::base::RequestId::FromRaw(12),
                              "blocked by unit test")
             .ok());
  first_state = tabs.GetTabState(first);
  second_state = tabs.GetTabState(second);
  assert(first_state != nullptr);
  assert(second_state != nullptr);
  assert(first_state->navigation_state == speed::browser::TabNavigationState::kCommitted);
  assert(first_state->current_url == "https://first.test");
  assert(second_state->navigation_state == speed::browser::TabNavigationState::kBlocked);
  assert(second_state->current_url.empty());
  assert(second_state->last_error == "blocked by unit test");

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
