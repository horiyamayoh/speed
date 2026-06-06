#pragma once

#include "base/result/status.h"

#include <cstdint>

namespace speed::platform
{

enum class ChildProcessKind : std::uint8_t
{
  kRenderer,
  kNetwork,
  kUtility,
};

class ProcessLauncher final
{
public:
  [[nodiscard]] base::Status LaunchStub(ChildProcessKind kind) const;
};

} // namespace speed::platform
