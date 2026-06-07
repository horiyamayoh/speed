#pragma once

#include "base/result/status.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace speed::platform
{

enum class ChildProcessKind : std::uint8_t
{
  kBrowser,
  kRenderer,
  kNetwork,
  kUtility,
};

struct ChildExitStatus final
{
  bool exited{false};
  int exit_code{0};
  bool signaled{false};
  int signal_number{0};
};

struct ChildProcessLaunchOptions final
{
  ChildProcessKind kind{ChildProcessKind::kUtility};
  std::filesystem::path executable_path;
  std::vector<std::string> arguments;
};

class ChildProcess final
{
public:
  ChildProcess() = default;
  explicit ChildProcess(int process_id);
  ~ChildProcess();

  ChildProcess(const ChildProcess&) = delete;
  ChildProcess& operator=(const ChildProcess&) = delete;
  ChildProcess(ChildProcess&& other) noexcept;
  ChildProcess& operator=(ChildProcess&& other) noexcept;

  [[nodiscard]] bool valid() const;
  [[nodiscard]] int process_id() const;
  [[nodiscard]] bool reaped() const;

  [[nodiscard]] base::Status TryWait(ChildExitStatus& exit_status, bool& has_exited);
  [[nodiscard]] base::Status WaitForExit(ChildExitStatus& exit_status);
  [[nodiscard]] base::Status WaitForExitFor(std::chrono::milliseconds timeout,
                                            ChildExitStatus& exit_status);
  [[nodiscard]] base::Status Terminate();

private:
  void Reset();

  int process_id_{-1};
  bool reaped_{true};
};

class ProcessLauncher final
{
public:
  [[nodiscard]] base::Status LaunchStub(ChildProcessKind kind) const;
  [[nodiscard]] base::Status Launch(const ChildProcessLaunchOptions& options,
                                    ChildProcess& child_process) const;
};

} // namespace speed::platform
