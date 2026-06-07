#include "network/network_process.h"

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
  output << "usage: speed-network [--ipc-fd=<fd>]\n";
}

} // namespace

int main(int argc, char* argv[])
{
  if (argc == 1)
  {
    return speed::network::RunNetworkProcess();
  }

  if (argc == 2)
  {
    const std::string_view argument(argv[1]);
    if (argument == "-h" || argument == "--help")
    {
      PrintUsage(std::cout);
      return 0;
    }

    int ipc_fd = -1;
    if (ParseIpcFd(argument, ipc_fd))
    {
      return speed::network::RunNetworkProcess(ipc_fd);
    }
  }

  PrintUsage(std::cerr);
  return 1;
}
