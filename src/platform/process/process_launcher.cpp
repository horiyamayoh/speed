#include "platform/process/process_launcher.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace speed::platform
{

namespace
{

[[nodiscard]] base::Status ErrnoStatus(std::string_view prefix)
{
  return base::Status::Error(std::string(prefix) + ": " + std::strerror(errno));
}

[[nodiscard]] ChildExitStatus DecodeWaitStatus(int wait_status)
{
  ChildExitStatus exit_status;
  if (WIFEXITED(wait_status))
  {
    exit_status.exited = true;
    exit_status.exit_code = WEXITSTATUS(wait_status);
  }
  else if (WIFSIGNALED(wait_status))
  {
    exit_status.signaled = true;
    exit_status.signal_number = WTERMSIG(wait_status);
  }
  return exit_status;
}

[[nodiscard]] bool IsKnownChildKind(ChildProcessKind kind)
{
  switch (kind)
  {
  case ChildProcessKind::kBrowser:
  case ChildProcessKind::kRenderer:
  case ChildProcessKind::kNetwork:
  case ChildProcessKind::kUtility:
    return true;
  }

  return false;
}

} // namespace

ChildProcess::ChildProcess(int process_id)
    : process_id_(process_id),
      reaped_(process_id <= 0)
{}

ChildProcess::~ChildProcess()
{
  Reset();
}

ChildProcess::ChildProcess(ChildProcess&& other) noexcept
    : process_id_(std::exchange(other.process_id_, -1)),
      reaped_(std::exchange(other.reaped_, true))
{}

ChildProcess& ChildProcess::operator=(ChildProcess&& other) noexcept
{
  if (this != &other)
  {
    Reset();
    process_id_ = std::exchange(other.process_id_, -1);
    reaped_ = std::exchange(other.reaped_, true);
  }
  return *this;
}

bool ChildProcess::valid() const
{
  return process_id_ > 0;
}

int ChildProcess::process_id() const
{
  return process_id_;
}

bool ChildProcess::reaped() const
{
  return reaped_;
}

base::Status ChildProcess::TryWait(ChildExitStatus& exit_status, bool& has_exited)
{
  has_exited = false;
  exit_status = {};

  if (!valid())
  {
    return base::Status::Error("child process is not valid");
  }

  if (reaped_)
  {
    return base::Status::Error("child process has already been reaped");
  }

  int wait_status = 0;
  const int result = ::waitpid(process_id_, &wait_status, WNOHANG);
  if (result < 0)
  {
    return ErrnoStatus("failed to wait for child process");
  }

  if (result == 0)
  {
    return base::Status::Ok();
  }

  has_exited = true;
  reaped_ = true;
  exit_status = DecodeWaitStatus(wait_status);
  return base::Status::Ok();
}

base::Status ChildProcess::WaitForExit(ChildExitStatus& exit_status)
{
  exit_status = {};

  if (!valid())
  {
    return base::Status::Error("child process is not valid");
  }

  if (reaped_)
  {
    return base::Status::Error("child process has already been reaped");
  }

  int wait_status = 0;
  for (;;)
  {
    const int result = ::waitpid(process_id_, &wait_status, 0);
    if (result < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      return ErrnoStatus("failed to wait for child process");
    }

    reaped_ = true;
    exit_status = DecodeWaitStatus(wait_status);
    return base::Status::Ok();
  }
}

base::Status ChildProcess::WaitForExitFor(std::chrono::milliseconds timeout,
                                          ChildExitStatus& exit_status)
{
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  for (;;)
  {
    bool has_exited = false;
    base::Status status = TryWait(exit_status, has_exited);
    if (!status.ok())
    {
      return status;
    }

    if (has_exited)
    {
      return base::Status::Ok();
    }

    if (std::chrono::steady_clock::now() >= deadline)
    {
      return base::Status::Error("timed out waiting for child process");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}

base::Status ChildProcess::Terminate()
{
  if (!valid())
  {
    return base::Status::Error("child process is not valid");
  }

  if (reaped_)
  {
    return base::Status::Ok();
  }

  if (::kill(process_id_, SIGTERM) != 0 && errno != ESRCH)
  {
    return ErrnoStatus("failed to terminate child process");
  }

  return base::Status::Ok();
}

void ChildProcess::Reset()
{
  if (!valid() || reaped_)
  {
    return;
  }

  (void)Terminate();
  ChildExitStatus exit_status;
  (void)WaitForExitFor(std::chrono::milliseconds(250), exit_status);
}

base::Status ProcessLauncher::LaunchStub(ChildProcessKind kind) const
{
  if (IsKnownChildKind(kind))
  {
    return base::Status::Ok();
  }

  return base::Status::Error("unknown child process kind");
}

base::Status ProcessLauncher::Launch(const ChildProcessLaunchOptions& options,
                                      ChildProcess& child_process) const
{
  if (!IsKnownChildKind(options.kind))
  {
    return base::Status::Error("unknown child process kind");
  }

  if (options.executable_path.empty())
  {
    return base::Status::Error("child process executable path is empty");
  }

  const std::string executable = options.executable_path.string();
  std::vector<std::string> argument_storage;
  argument_storage.reserve(options.arguments.size() + 1);
  argument_storage.push_back(executable);
  for (const std::string& argument : options.arguments)
  {
    argument_storage.push_back(argument);
  }

  std::vector<char*> argv;
  argv.reserve(argument_storage.size() + 1);
  for (std::string& argument : argument_storage)
  {
    argv.push_back(argument.data());
  }
  argv.push_back(nullptr);

  const pid_t pid = ::fork();
  if (pid < 0)
  {
    return ErrnoStatus("failed to fork child process");
  }

  if (pid == 0)
  {
    ::execv(executable.c_str(), argv.data());
    _exit(127);
  }

  child_process = ChildProcess(static_cast<int>(pid));
  return base::Status::Ok();
}

} // namespace speed::platform
