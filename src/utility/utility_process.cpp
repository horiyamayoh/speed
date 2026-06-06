#include "utility/utility_process.h"

#include "base/logging/logging.h"

namespace speed::utility
{

int RunUtilityProcess()
{
  base::Log(base::LogLevel::kInfo, "utility", "utility process stub ready");
  return 0;
}

} // namespace speed::utility
