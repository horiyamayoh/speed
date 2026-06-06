#pragma once

#include <filesystem>

namespace speed::storage
{

class ProfileDirectory final
{
public:
  explicit ProfileDirectory(std::filesystem::path root);

  [[nodiscard]] const std::filesystem::path& root() const;

private:
  std::filesystem::path root_;
};

} // namespace speed::storage
