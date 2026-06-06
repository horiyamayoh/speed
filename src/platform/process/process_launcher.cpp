#include "platform/process/process_launcher.h"

namespace speed::platform
{

base::Status ProcessLauncher::LaunchStub(ChildProcessKind kind) const
{
  switch (kind)
  {
  case ChildProcessKind::kRenderer:
  case ChildProcessKind::kNetwork:
  case ChildProcessKind::kUtility:
    return base::Status::Ok();
  }

  return base::Status::Error("unknown child process kind");
}

} // namespace speed::platform
