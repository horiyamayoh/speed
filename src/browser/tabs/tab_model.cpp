#include "browser/tabs/tab_model.h"

#include <algorithm>
#include <iterator>

namespace speed::browser
{

base::TabId TabModel::CreateTab()
{
  const base::TabId tab_id = base::TabId::FromRaw(next_tab_id_++);
  tabs_.push_back(tab_id);
  active_tab_ = tab_id;
  return tab_id;
}

bool TabModel::SwitchToTab(base::TabId tab_id)
{
  if (!ContainsTab(tab_id))
  {
    return false;
  }

  active_tab_ = tab_id;
  return true;
}

bool TabModel::CloseTab(base::TabId tab_id)
{
  const auto tab = std::find(tabs_.begin(), tabs_.end(), tab_id);
  if (tab == tabs_.end())
  {
    return false;
  }

  if (active_tab_ == tab_id)
  {
    const auto next_tab = std::next(tab);
    if (next_tab != tabs_.end())
    {
      active_tab_ = *next_tab;
    }
    else if (tab != tabs_.begin())
    {
      active_tab_ = *std::prev(tab);
    }
    else
    {
      active_tab_ = {};
    }
  }

  tabs_.erase(tab);
  return true;
}

void TabModel::CloseAllTabs()
{
  tabs_.clear();
  active_tab_ = {};
}

bool TabModel::ContainsTab(base::TabId tab_id) const
{
  return std::find(tabs_.begin(), tabs_.end(), tab_id) != tabs_.end();
}

std::size_t TabModel::tab_count() const
{
  return tabs_.size();
}

base::TabId TabModel::active_tab() const
{
  return active_tab_;
}

} // namespace speed::browser
