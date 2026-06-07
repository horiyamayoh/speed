#include "browser/tabs/tab_model.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace speed::browser
{

base::TabId TabModel::CreateTab()
{
  const base::TabId tab_id = base::TabId::FromRaw(next_tab_id_++);
  tabs_.push_back({
      .id = tab_id,
      .navigation_state = TabNavigationState::kEmpty,
      .pending_request_id = {},
      .current_document_id = {},
      .pending_url = {},
      .current_url = {},
      .last_error = {},
  });
  active_tab_ = tab_id;
  return tab_id;
}

bool TabModel::SwitchToTab(base::TabId tab_id)
{
  if (FindTab(tab_id) == tabs_.end())
  {
    return false;
  }

  active_tab_ = tab_id;
  return true;
}

bool TabModel::CloseTab(base::TabId tab_id)
{
  const auto tab = FindTab(tab_id);
  if (tab == tabs_.end())
  {
    return false;
  }

  if (active_tab_ == tab_id)
  {
    const auto next_tab = std::next(tab);
    if (next_tab != tabs_.end())
    {
      active_tab_ = next_tab->id;
    }
    else if (tab != tabs_.begin())
    {
      active_tab_ = std::prev(tab)->id;
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
  return FindTab(tab_id) != tabs_.end();
}

const TabState* TabModel::GetTabState(base::TabId tab_id) const
{
  const auto tab = FindTab(tab_id);
  if (tab == tabs_.end())
  {
    return nullptr;
  }

  return &*tab;
}

std::size_t TabModel::tab_count() const
{
  return tabs_.size();
}

base::TabId TabModel::active_tab() const
{
  return active_tab_;
}

base::Status
TabModel::StartNavigation(base::TabId tab_id, base::RequestId request_id, std::string url)
{
  if (!request_id)
  {
    return base::Status::Error("navigation request id is invalid");
  }

  if (url.empty())
  {
    return base::Status::Error("navigation URL is empty");
  }

  const auto tab = FindTab(tab_id);
  if (tab == tabs_.end())
  {
    return base::Status::Error("tab does not exist");
  }

  tab->navigation_state = TabNavigationState::kLoading;
  tab->pending_request_id = request_id;
  tab->pending_url = std::move(url);
  tab->last_error.clear();
  return base::Status::Ok();
}

base::Status TabModel::CommitNavigation(base::TabId tab_id,
                                        base::RequestId request_id,
                                        base::DocumentId document_id,
                                        std::string url)
{
  if (!document_id)
  {
    return base::Status::Error("document id is invalid");
  }

  const auto tab = FindTab(tab_id);
  if (tab == tabs_.end())
  {
    return base::Status::Error("tab does not exist");
  }

  if (tab->pending_request_id != request_id)
  {
    return base::Status::Error("navigation response does not match pending request");
  }

  tab->navigation_state = TabNavigationState::kCommitted;
  tab->pending_request_id = {};
  tab->current_document_id = document_id;
  tab->pending_url.clear();
  tab->current_url = std::move(url);
  tab->last_error.clear();
  return base::Status::Ok();
}

base::Status
TabModel::BlockNavigation(base::TabId tab_id, base::RequestId request_id, std::string reason)
{
  const auto tab = FindTab(tab_id);
  if (tab == tabs_.end())
  {
    return base::Status::Error("tab does not exist");
  }

  if (tab->pending_request_id != request_id)
  {
    return base::Status::Error("navigation block does not match pending request");
  }

  tab->navigation_state = TabNavigationState::kBlocked;
  tab->pending_request_id = {};
  tab->pending_url.clear();
  tab->last_error = reason.empty() ? "navigation blocked" : std::move(reason);
  return base::Status::Ok();
}

base::Status
TabModel::FailNavigation(base::TabId tab_id, base::RequestId request_id, std::string reason)
{
  const auto tab = FindTab(tab_id);
  if (tab == tabs_.end())
  {
    return base::Status::Error("tab does not exist");
  }

  if (tab->pending_request_id != request_id)
  {
    return base::Status::Error("navigation failure does not match pending request");
  }

  tab->navigation_state = TabNavigationState::kFailed;
  tab->pending_request_id = {};
  tab->pending_url.clear();
  tab->last_error = reason.empty() ? "navigation failed" : std::move(reason);
  return base::Status::Ok();
}

base::Status TabModel::MarkCrashed(base::TabId tab_id, std::string reason)
{
  const auto tab = FindTab(tab_id);
  if (tab == tabs_.end())
  {
    return base::Status::Error("tab does not exist");
  }

  tab->navigation_state = TabNavigationState::kCrashed;
  tab->pending_request_id = {};
  tab->pending_url.clear();
  tab->last_error = reason.empty() ? "renderer crashed" : std::move(reason);
  return base::Status::Ok();
}

std::vector<TabState>::iterator TabModel::FindTab(base::TabId tab_id)
{
  return std::ranges::find(tabs_, tab_id, &TabState::id);
}

std::vector<TabState>::const_iterator TabModel::FindTab(base::TabId tab_id) const
{
  return std::ranges::find(tabs_, tab_id, &TabState::id);
}

} // namespace speed::browser
