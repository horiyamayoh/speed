#include "storage/history/history_store.h"

#include "storage/profile/profile_directory.h"

#include <fstream>
#include <ios>
#include <system_error>
#include <utility>

namespace speed::storage
{
namespace
{

constexpr char kHistoryFormatHeader[] = "SPEED_HISTORY_V1";
constexpr char kHistoryFileName[] = "history.urls";

std::string EscapeHistoryEntry(const std::string& entry)
{
  std::string escaped;
  escaped.reserve(entry.size());

  for (const char character : entry)
  {
    switch (character)
    {
    case '\\':
      escaped += "\\\\";
      break;
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      escaped += "\\r";
      break;
    default:
      escaped.push_back(character);
      break;
    }
  }

  return escaped;
}

bool UnescapeHistoryEntry(const std::string& escaped, std::string& entry)
{
  entry.clear();
  entry.reserve(escaped.size());

  bool pending_escape = false;
  for (const char character : escaped)
  {
    if (!pending_escape)
    {
      if (character == '\\')
      {
        pending_escape = true;
      }
      else
      {
        entry.push_back(character);
      }
      continue;
    }

    switch (character)
    {
    case '\\':
      entry.push_back('\\');
      break;
    case 'n':
      entry.push_back('\n');
      break;
    case 'r':
      entry.push_back('\r');
      break;
    default:
      return false;
    }

    pending_escape = false;
  }

  return !pending_escape;
}

} // namespace

HistoryStore::HistoryStore(const ProfileDirectory& profile_directory)
    : history_file_path_(HistoryFilePathFor(profile_directory))
{}

HistoryStore::HistoryStore(std::filesystem::path history_file_path)
    : history_file_path_(std::move(history_file_path))
{}

base::Status HistoryStore::Open(const ProfileDirectory& profile_directory)
{
  return Open(HistoryFilePathFor(profile_directory));
}

base::Status HistoryStore::Open(std::filesystem::path history_file_path)
{
  if (history_file_path.empty())
  {
    return base::Status::Error("history file path must not be empty");
  }

  history_file_path_ = std::move(history_file_path);
  return Load();
}

base::Status HistoryStore::Load()
{
  if (history_file_path_.empty())
  {
    return base::Status::Error("history store has no durable file path");
  }

  std::error_code exists_error;
  const bool history_file_exists = std::filesystem::exists(history_file_path_, exists_error);
  if (exists_error)
  {
    return base::Status::Error("failed to inspect history file: " + exists_error.message());
  }

  if (!history_file_exists)
  {
    entries_.clear();
    return base::Status::Ok();
  }

  std::ifstream history_file(history_file_path_);
  if (!history_file)
  {
    return base::Status::Error("failed to open history file for reading");
  }

  std::string header;
  if (!std::getline(history_file, header))
  {
    if (history_file.bad())
    {
      return base::Status::Error("failed to read history file header");
    }

    return base::Status::Error("history file is empty");
  }

  if (header != kHistoryFormatHeader)
  {
    return base::Status::Error("unsupported history file format");
  }

  std::vector<std::string> loaded_entries;
  std::string line;
  while (std::getline(history_file, line))
  {
    std::string entry;
    if (!UnescapeHistoryEntry(line, entry))
    {
      return base::Status::Error("history file contains an invalid escaped URL");
    }

    if (entry.empty())
    {
      return base::Status::Error("history file contains an empty URL");
    }

    loaded_entries.push_back(std::move(entry));
  }

  if (history_file.bad())
  {
    return base::Status::Error("failed to read history file entries");
  }

  entries_ = std::move(loaded_entries);
  return base::Status::Ok();
}

base::Status HistoryStore::Flush() const
{
  if (history_file_path_.empty())
  {
    return base::Status::Error("history store has no durable file path");
  }

  const std::filesystem::path parent_path = history_file_path_.parent_path();
  if (!parent_path.empty())
  {
    std::error_code create_error;
    std::filesystem::create_directories(parent_path, create_error);
    if (create_error)
    {
      return base::Status::Error("failed to create history directory: " + create_error.message());
    }
  }

  std::ofstream history_file(history_file_path_, std::ios::out | std::ios::trunc);
  if (!history_file)
  {
    return base::Status::Error("failed to open history file for writing");
  }

  history_file << kHistoryFormatHeader << '\n';
  for (const std::string& entry : entries_)
  {
    history_file << EscapeHistoryEntry(entry) << '\n';
  }

  if (!history_file)
  {
    return base::Status::Error("failed to write history file");
  }

  return base::Status::Ok();
}

base::Status HistoryStore::RecordVisit(std::string url)
{
  if (url.empty())
  {
    return base::Status::Error("history URL must not be empty");
  }

  entries_.push_back(std::move(url));
  return base::Status::Ok();
}

bool HistoryStore::durable() const
{
  return !history_file_path_.empty();
}

std::size_t HistoryStore::entry_count() const
{
  return entries_.size();
}

const std::vector<std::string>& HistoryStore::entries() const
{
  return entries_;
}

std::filesystem::path HistoryStore::HistoryFilePathFor(const ProfileDirectory& profile_directory)
{
  return profile_directory.root() / kHistoryFileName;
}

} // namespace speed::storage
