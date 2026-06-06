#include "storage/profile/profile_directory.h"

#include <utility>

namespace speed::storage
{

ProfileDirectory::ProfileDirectory(std::filesystem::path root)
    : root_(std::move(root))
{}

const std::filesystem::path& ProfileDirectory::root() const
{
  return root_;
}

} // namespace speed::storage
