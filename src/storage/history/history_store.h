#pragma once

#include "base/result/status.h"

#include <cstddef>
#include <string>
#include <vector>

namespace speed::storage
{

class HistoryStore final
{
public:
  [[nodiscard]] base::Status RecordVisit(std::string url);

  [[nodiscard]] std::size_t entry_count() const;
  [[nodiscard]] const std::vector<std::string>& entries() const;

private:
  std::vector<std::string> entries_;
};

} // namespace speed::storage
