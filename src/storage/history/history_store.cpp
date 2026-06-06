#include "storage/history/history_store.h"

#include <utility>

namespace speed::storage
{

base::Status HistoryStore::RecordVisit(std::string url)
{
  if (url.empty())
  {
    return base::Status::Error("history URL must not be empty");
  }

  entries_.push_back(std::move(url));
  return base::Status::Ok();
}

std::size_t HistoryStore::entry_count() const
{
  return entries_.size();
}

const std::vector<std::string>& HistoryStore::entries() const
{
  return entries_;
}

} // namespace speed::storage
