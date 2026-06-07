#pragma once

#include "base/ids/id_types.h"
#include "base/result/status.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace speed::browser
{

enum class TabNavigationState : std::uint8_t
{
  kEmpty,
  kLoading,
  kCommitted,
  kBlocked,
  kFailed,
  kCrashed,
};

struct TabState final
{
  base::TabId id;
  TabNavigationState navigation_state{TabNavigationState::kEmpty};
  base::RequestId pending_request_id;
  base::DocumentId current_document_id;
  std::string pending_url;
  std::string current_url;
  std::string last_error;
};

class TabModel final
{
public:
  [[nodiscard]] base::TabId CreateTab();
  [[nodiscard]] bool SwitchToTab(base::TabId tab_id);
  [[nodiscard]] bool CloseTab(base::TabId tab_id);
  void CloseAllTabs();

  [[nodiscard]] bool ContainsTab(base::TabId tab_id) const;
  [[nodiscard]] const TabState* GetTabState(base::TabId tab_id) const;
  [[nodiscard]] std::size_t tab_count() const;
  [[nodiscard]] base::TabId active_tab() const;

  [[nodiscard]] base::Status
  StartNavigation(base::TabId tab_id, base::RequestId request_id, std::string url);
  [[nodiscard]] base::Status CommitNavigation(base::TabId tab_id,
                                              base::RequestId request_id,
                                              base::DocumentId document_id,
                                              std::string url);
  [[nodiscard]] base::Status
  BlockNavigation(base::TabId tab_id, base::RequestId request_id, std::string reason);
  [[nodiscard]] base::Status
  FailNavigation(base::TabId tab_id, base::RequestId request_id, std::string reason);
  [[nodiscard]] base::Status MarkCrashed(base::TabId tab_id, std::string reason);

private:
  [[nodiscard]] std::vector<TabState>::iterator FindTab(base::TabId tab_id);
  [[nodiscard]] std::vector<TabState>::const_iterator FindTab(base::TabId tab_id) const;

  std::uint64_t next_tab_id_{1};
  std::vector<TabState> tabs_;
  base::TabId active_tab_;
};

} // namespace speed::browser
