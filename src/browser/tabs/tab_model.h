#pragma once

#include "base/ids/id_types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace speed::browser
{

class TabModel final
{
public:
  [[nodiscard]] base::TabId CreateTab();
  [[nodiscard]] bool SwitchToTab(base::TabId tab_id);
  [[nodiscard]] bool CloseTab(base::TabId tab_id);
  void CloseAllTabs();

  [[nodiscard]] bool ContainsTab(base::TabId tab_id) const;
  [[nodiscard]] std::size_t tab_count() const;
  [[nodiscard]] base::TabId active_tab() const;

private:
  std::uint64_t next_tab_id_{1};
  std::vector<base::TabId> tabs_;
  base::TabId active_tab_;
};

} // namespace speed::browser
