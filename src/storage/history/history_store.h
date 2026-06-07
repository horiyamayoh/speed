#pragma once

#include "base/result/status.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace speed::storage
{

class ProfileDirectory;

class HistoryStore final
{
public:
  HistoryStore() = default;
  explicit HistoryStore(const ProfileDirectory& profile_directory);
  explicit HistoryStore(std::filesystem::path history_file_path);

  [[nodiscard]] base::Status Open(const ProfileDirectory& profile_directory);
  [[nodiscard]] base::Status Open(std::filesystem::path history_file_path);
  [[nodiscard]] base::Status Load();
  [[nodiscard]] base::Status Flush() const;

  [[nodiscard]] base::Status RecordVisit(std::string url);

  [[nodiscard]] bool durable() const;
  [[nodiscard]] std::size_t entry_count() const;
  [[nodiscard]] const std::vector<std::string>& entries() const;

private:
  [[nodiscard]] static std::filesystem::path
  HistoryFilePathFor(const ProfileDirectory& profile_directory);

  std::filesystem::path history_file_path_;
  std::vector<std::string> entries_;
};

} // namespace speed::storage
