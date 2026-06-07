#pragma once

#include "base/ids/id_types.h"
#include "base/result/status.h"
#include "platform/process/process_launcher.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace speed::browser
{

enum class ChildProcessRole : std::uint8_t
{
  kRenderer,
  kNetwork,
  kUtility,
};

enum class ChildProcessState : std::uint8_t
{
  kCreated,
  kRunning,
  kExited,
  kCrashed,
  kLaunchFailed,
};

struct ChildProcessHostOptions final
{
  ChildProcessRole role{ChildProcessRole::kUtility};
  std::filesystem::path executable_path;
  std::vector<std::string> arguments;
  base::TabId tab_id;
};

class ChildProcessHost final
{
public:
  ChildProcessHost() = default;
  explicit ChildProcessHost(ChildProcessHostOptions options);
  ~ChildProcessHost();

  ChildProcessHost(const ChildProcessHost&) = delete;
  ChildProcessHost& operator=(const ChildProcessHost&) = delete;
  ChildProcessHost(ChildProcessHost&& other) noexcept;
  ChildProcessHost& operator=(ChildProcessHost&& other) noexcept;

  [[nodiscard]] base::Status Launch();
  [[nodiscard]] base::Status PollForExit();
  [[nodiscard]] base::Status WaitForExitFor(std::chrono::milliseconds timeout);
  [[nodiscard]] base::Status TerminateForShutdown(std::chrono::milliseconds timeout);

  [[nodiscard]] ChildProcessRole role() const;
  [[nodiscard]] ChildProcessState state() const;
  [[nodiscard]] base::TabId tab_id() const;
  [[nodiscard]] int process_id() const;
  [[nodiscard]] const std::string& last_error() const;
  [[nodiscard]] std::optional<platform::ChildExitStatus> exit_status() const;
  [[nodiscard]] std::string CrashReason() const;

private:
  [[nodiscard]] platform::ChildProcessKind PlatformKind() const;
  void RecordExit(platform::ChildExitStatus exit_status, bool expected_shutdown);
  void RecordLaunchFailure(std::string message);

  ChildProcessHostOptions options_;
  platform::ChildProcess child_;
  ChildProcessState state_{ChildProcessState::kCreated};
  std::optional<platform::ChildExitStatus> exit_status_;
  std::string last_error_;
};

} // namespace speed::browser
