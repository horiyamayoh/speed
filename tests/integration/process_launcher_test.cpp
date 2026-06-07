#include "platform/process/process_launcher.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace
{

[[nodiscard]] std::filesystem::path AppPath(std::string executable_name)
{
  return std::filesystem::path(SPEED_APP_DIR) / executable_name;
}

void ExpectCleanExit(const std::filesystem::path& executable_path,
                     speed::platform::ChildProcessKind kind,
                     std::vector<std::string> arguments = {})
{
  speed::platform::ProcessLauncher launcher;
  speed::platform::ChildProcess child;
  speed::base::Status status = launcher.Launch(
      {
          .kind = kind,
          .executable_path = executable_path,
          .arguments = std::move(arguments),
      },
      child);
  assert(status.ok());
  assert(child.valid());
  assert(child.process_id() > 0);

  speed::platform::ChildExitStatus exit_status;
  status = child.WaitForExitFor(std::chrono::seconds(5), exit_status);
  assert(status.ok());
  assert(exit_status.exited);
  assert(exit_status.exit_code == 0);
  assert(!exit_status.signaled);
  assert(child.reaped());
}

void TimesOutAndTerminatesLongRunningChild()
{
  const std::filesystem::path sleep_path = "/bin/sleep";
  if (!std::filesystem::exists(sleep_path))
  {
    return;
  }

  speed::platform::ProcessLauncher launcher;
  speed::platform::ChildProcess child;
  speed::base::Status status = launcher.Launch(
      {
          .kind = speed::platform::ChildProcessKind::kUtility,
          .executable_path = sleep_path,
          .arguments = {"10"},
      },
      child);
  assert(status.ok());

  speed::platform::ChildExitStatus exit_status;
  status = child.WaitForExitFor(std::chrono::milliseconds(20), exit_status);
  assert(!status.ok());

  status = child.Terminate();
  assert(status.ok());
  status = child.WaitForExitFor(std::chrono::seconds(5), exit_status);
  assert(status.ok());
  assert(exit_status.signaled || exit_status.exited);
}

} // namespace

int main()
{
  const speed::platform::ProcessLauncher launcher;
  assert(launcher.LaunchStub(speed::platform::ChildProcessKind::kBrowser).ok());
  assert(launcher.LaunchStub(speed::platform::ChildProcessKind::kRenderer).ok());
  assert(launcher.LaunchStub(speed::platform::ChildProcessKind::kNetwork).ok());
  assert(launcher.LaunchStub(speed::platform::ChildProcessKind::kUtility).ok());

  ExpectCleanExit(
      AppPath("speed-browser"), speed::platform::ChildProcessKind::kBrowser, {"--help"});
  ExpectCleanExit(AppPath("speed-renderer"), speed::platform::ChildProcessKind::kRenderer);
  ExpectCleanExit(AppPath("speed-network"), speed::platform::ChildProcessKind::kNetwork);
  ExpectCleanExit(AppPath("speed-utility"), speed::platform::ChildProcessKind::kUtility);
  TimesOutAndTerminatesLongRunningChild();

  return 0;
}
