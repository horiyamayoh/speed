#include "renderer/document/renderer_process.h"

#include <charconv>
#include <iostream>
#include <string_view>
#include <system_error>

namespace
{

[[nodiscard]] bool ParseIpcFd(std::string_view argument, int& ipc_fd)
{
  constexpr std::string_view prefix = "--ipc-fd=";
  if (!argument.starts_with(prefix))
  {
    return false;
  }

  const std::string_view value = argument.substr(prefix.size());
  const char* const begin = value.data();
  const char* const end = value.data() + value.size();
  int parsed = -1;
  const std::from_chars_result result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end || parsed < 0)
  {
    return false;
  }

  ipc_fd = parsed;
  return true;
}

void PrintUsage(std::ostream& output)
{
  output << "usage: speed-renderer [--ipc-fd=<fd>] [--crash-after-commit-count=<n>]\n";
}

[[nodiscard]] bool ParseCrashAfterCommitCount(std::string_view argument, int& commit_count)
{
  constexpr std::string_view prefix = "--crash-after-commit-count=";
  if (!argument.starts_with(prefix))
  {
    return false;
  }

  const std::string_view value = argument.substr(prefix.size());
  const char* const begin = value.data();
  const char* const end = value.data() + value.size();
  int parsed = 0;
  const std::from_chars_result result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end || parsed <= 0)
  {
    return false;
  }

  commit_count = parsed;
  return true;
}

} // namespace

int main(int argc, char* argv[])
{
  int ipc_fd = -1;
  int crash_after_commit_count = 0;
  for (int index = 1; index < argc; ++index)
  {
    const std::string_view argument(argv[index]);
    if (argument == "-h" || argument == "--help")
    {
      PrintUsage(std::cout);
      return 0;
    }

    if (ParseIpcFd(argument, ipc_fd))
    {
      continue;
    }

    if (ParseCrashAfterCommitCount(argument, crash_after_commit_count))
    {
      continue;
    }

    PrintUsage(std::cerr);
    return 1;
  }

  return speed::renderer::RunRendererProcess(ipc_fd, crash_after_commit_count);
}
