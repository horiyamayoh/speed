#include "network/network_process.h"

#include "base/logging/logging.h"

namespace speed::network
{

int RunNetworkProcess()
{
  base::Log(base::LogLevel::kInfo, "network", "network process stub ready");
  return 0;
}

} // namespace speed::network
