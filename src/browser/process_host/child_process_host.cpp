#include "browser/process_host/child_process_host.h"

#include <chrono>
#include <string>
#include <utility>

namespace speed::browser
{

namespace
{

[[nodiscard]] std::string ExitStatusReason(const platform::ChildExitStatus& status)
{
  if (status.exited)
  {
    return "child process exited with code " + std::to_string(status.exit_code);
  }

  if (status.signaled)
  {
    return "child process exited from signal " + std::to_string(status.signal_number);
  }

  return "child process exited";
}

} // namespace

ChildProcessHost::ChildProcessHost(ChildProcessHostOptions options)
    : options_(std::move(options))
{}

ChildProcessHost::~ChildProcessHost()
{
  if (state_ == ChildProcessState::kRunning)
  {
    (void)TerminateForShutdown(std::chrono::milliseconds(250));
  }
}

ChildProcessHost::ChildProcessHost(ChildProcessHost&& other) noexcept
    : options_(std::move(other.options_)),
      child_(std::move(other.child_)),
      state_(std::exchange(other.state_, ChildProcessState::kExited)),
      exit_status_(std::move(other.exit_status_)),
      last_error_(std::move(other.last_error_))
{}

ChildProcessHost& ChildProcessHost::operator=(ChildProcessHost&& other) noexcept
{
  if (this != &other)
  {
    if (state_ == ChildProcessState::kRunning)
    {
      (void)TerminateForShutdown(std::chrono::milliseconds(250));
    }

    options_ = std::move(other.options_);
    child_ = std::move(other.child_);
    state_ = std::exchange(other.state_, ChildProcessState::kExited);
    exit_status_ = std::move(other.exit_status_);
    last_error_ = std::move(other.last_error_);
  }
  return *this;
}

base::Status ChildProcessHost::Launch()
{
  if (state_ == ChildProcessState::kRunning)
  {
    return base::Status::Error("child process is already running");
  }

  if (state_ != ChildProcessState::kCreated)
  {
    return base::Status::Error("child process host cannot be relaunched");
  }

  platform::ProcessLauncher launcher;
  base::Status status = launcher.Launch(
      {
          .kind = PlatformKind(),
          .executable_path = options_.executable_path,
          .arguments = options_.arguments,
      },
      child_);
  if (!status.ok())
  {
    RecordLaunchFailure(status.message());
    return status;
  }

  state_ = ChildProcessState::kRunning;
  last_error_.clear();
  return base::Status::Ok();
}

base::Status ChildProcessHost::PollForExit()
{
  if (state_ != ChildProcessState::kRunning)
  {
    return base::Status::Ok();
  }

  platform::ChildExitStatus exit_status;
  bool has_exited = false;
  base::Status status = child_.TryWait(exit_status, has_exited);
  if (!status.ok())
  {
    last_error_ = status.message();
    return status;
  }

  if (has_exited)
  {
    RecordExit(exit_status, false);
  }

  return base::Status::Ok();
}

base::Status ChildProcessHost::WaitForExitFor(std::chrono::milliseconds timeout)
{
  if (state_ != ChildProcessState::kRunning)
  {
    return base::Status::Ok();
  }

  platform::ChildExitStatus exit_status;
  base::Status status = child_.WaitForExitFor(timeout, exit_status);
  if (!status.ok())
  {
    last_error_ = status.message();
    return status;
  }

  RecordExit(exit_status, false);
  return base::Status::Ok();
}

base::Status ChildProcessHost::TerminateForShutdown(std::chrono::milliseconds timeout)
{
  if (state_ != ChildProcessState::kRunning)
  {
    return base::Status::Ok();
  }

  base::Status status = child_.Terminate();
  if (!status.ok())
  {
    last_error_ = status.message();
    return status;
  }

  platform::ChildExitStatus exit_status;
  status = child_.WaitForExitFor(timeout, exit_status);
  if (!status.ok())
  {
    last_error_ = status.message();
    return status;
  }

  RecordExit(exit_status, true);
  return base::Status::Ok();
}

ChildProcessRole ChildProcessHost::role() const
{
  return options_.role;
}

ChildProcessState ChildProcessHost::state() const
{
  return state_;
}

base::TabId ChildProcessHost::tab_id() const
{
  return options_.tab_id;
}

int ChildProcessHost::process_id() const
{
  return child_.process_id();
}

const std::string& ChildProcessHost::last_error() const
{
  return last_error_;
}

std::optional<platform::ChildExitStatus> ChildProcessHost::exit_status() const
{
  return exit_status_;
}

std::string ChildProcessHost::CrashReason() const
{
  if (!last_error_.empty())
  {
    return last_error_;
  }

  if (exit_status_.has_value())
  {
    return ExitStatusReason(*exit_status_);
  }

  return "child process crashed";
}

platform::ChildProcessKind ChildProcessHost::PlatformKind() const
{
  switch (options_.role)
  {
  case ChildProcessRole::kRenderer:
    return platform::ChildProcessKind::kRenderer;
  case ChildProcessRole::kNetwork:
    return platform::ChildProcessKind::kNetwork;
  case ChildProcessRole::kUtility:
    return platform::ChildProcessKind::kUtility;
  }

  return platform::ChildProcessKind::kUtility;
}

void ChildProcessHost::RecordExit(platform::ChildExitStatus exit_status, bool expected_shutdown)
{
  exit_status_ = exit_status;
  if (expected_shutdown || (exit_status.exited && exit_status.exit_code == 0))
  {
    state_ = ChildProcessState::kExited;
    last_error_.clear();
    return;
  }

  state_ = ChildProcessState::kCrashed;
  last_error_ = ExitStatusReason(exit_status);
}

void ChildProcessHost::RecordLaunchFailure(std::string message)
{
  state_ = ChildProcessState::kLaunchFailed;
  last_error_ = std::move(message);
}

} // namespace speed::browser
