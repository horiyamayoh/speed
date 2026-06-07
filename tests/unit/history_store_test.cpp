#include "storage/history/history_store.h"
#include "storage/profile/profile_directory.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace
{

class TempProfile final
{
public:
  TempProfile()
      : root_(std::filesystem::temp_directory_path() /
              ("speed_history_store_test_" +
               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
  {}

  TempProfile(const TempProfile&) = delete;
  TempProfile& operator=(const TempProfile&) = delete;

  ~TempProfile()
  {
    std::error_code ignored;
    std::filesystem::remove_all(root_, ignored);
  }

  [[nodiscard]] const std::filesystem::path& root() const
  {
    return root_;
  }

private:
  std::filesystem::path root_;
};

} // namespace

int main()
{
  speed::storage::HistoryStore memory_store;
  assert(memory_store.RecordVisit("https://memory.example/").ok());
  assert(memory_store.entry_count() == 1);
  assert(memory_store.entries().front() == "https://memory.example/");
  assert(!memory_store.Flush().ok());

  TempProfile temp_profile;
  const speed::storage::ProfileDirectory profile(temp_profile.root());

  speed::storage::HistoryStore store;
  speed::base::Status status = store.Open(profile);
  assert(status.ok());
  assert(store.entry_count() == 0);

  const std::vector<std::string> expected_entries = {
      "about:blank",
      "https://example.test/path?query=1",
      "https://example.test/backslash\\path",
  };

  for (const std::string& entry : expected_entries)
  {
    status = store.RecordVisit(entry);
    assert(status.ok());
  }

  status = store.Flush();
  assert(status.ok());

  speed::storage::HistoryStore loaded_from_profile(profile);
  status = loaded_from_profile.Load();
  assert(status.ok());
  assert(loaded_from_profile.entries() == expected_entries);

  const std::filesystem::path direct_history_path = temp_profile.root() / "direct.urls";
  speed::storage::HistoryStore direct_store(direct_history_path);
  status = direct_store.RecordVisit("https://direct.example/");
  assert(status.ok());
  status = direct_store.Flush();
  assert(status.ok());

  speed::storage::HistoryStore loaded_from_path;
  status = loaded_from_path.Open(direct_history_path);
  assert(status.ok());
  assert(loaded_from_path.entry_count() == 1);
  assert(loaded_from_path.entries().front() == "https://direct.example/");

  return 0;
}
