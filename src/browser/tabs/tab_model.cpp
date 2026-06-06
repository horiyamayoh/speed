#include "browser/tabs/tab_model.h"

#include <algorithm>

namespace speed::browser
{

base::TabId TabModel::CreateTab()
{
  const base::TabId tab_id = base::TabId::FromRaw(next_tab_id_++);
  tabs_.push_back(tab_id);
  active_tab_ = tab_id;
  return tab_id;
}

bool TabModel::CloseTab(base::TabId tab_id)
{
  const auto erased = std::erase(tabs_, tab_id);
  if (erased == 0)
  {
    return false;
  }

  if (active_tab_ == tab_id)
  {
    active_tab_ = tabs_.empty() ? base::TabId{} : tabs_.front();
  }

  return true;
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
