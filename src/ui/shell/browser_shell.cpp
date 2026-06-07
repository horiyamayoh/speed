#include "ui/shell/browser_shell.h"

#include "base/logging/logging.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace speed::ui
{

namespace
{

[[nodiscard]] bool IsAsciiWhitespace(char character)
{
  return character == ' ' || character == '\t' || character == '\n' || character == '\r' ||
         character == '\f' || character == '\v';
}

[[nodiscard]] char ToAsciiLower(char character)
{
  if (character >= 'A' && character <= 'Z')
  {
    return static_cast<char>(character - 'A' + 'a');
  }

  return character;
}

[[nodiscard]] std::string ToAsciiLower(std::string_view input)
{
  std::string result(input);
  for (char& character : result)
  {
    character = ToAsciiLower(character);
  }
  return result;
}

[[nodiscard]] std::string_view TrimAsciiWhitespace(std::string_view input)
{
  std::size_t begin = 0;
  while (begin < input.size() && IsAsciiWhitespace(input[begin]))
  {
    ++begin;
  }

  std::size_t end = input.size();
  while (end > begin && IsAsciiWhitespace(input[end - 1]))
  {
    --end;
  }

  return input.substr(begin, end - begin);
}

[[nodiscard]] std::string_view FirstToken(std::string_view input)
{
  const std::string_view trimmed = TrimAsciiWhitespace(input);
  const std::size_t token_end = trimmed.find_first_of(" \t\r\n\f\v");
  if (token_end == std::string_view::npos)
  {
    return trimmed;
  }

  return trimmed.substr(0, token_end);
}

[[nodiscard]] std::string_view RemainingInput(std::string_view input)
{
  const std::string_view trimmed = TrimAsciiWhitespace(input);
  const std::size_t token_end = trimmed.find_first_of(" \t\r\n\f\v");
  if (token_end == std::string_view::npos)
  {
    return {};
  }

  return TrimAsciiWhitespace(trimmed.substr(token_end));
}

[[nodiscard]] std::optional<base::TabId> ParseTabId(std::string_view input)
{
  const std::string_view trimmed = TrimAsciiWhitespace(input);
  if (trimmed.empty())
  {
    return std::nullopt;
  }

  std::uint64_t raw_id = 0;
  const char* const begin = trimmed.data();
  const char* const end = trimmed.data() + trimmed.size();
  const std::from_chars_result result = std::from_chars(begin, end, raw_id);
  if (result.ec != std::errc{} || result.ptr != end || raw_id == 0)
  {
    return std::nullopt;
  }

  return base::TabId::FromRaw(raw_id);
}

[[nodiscard]] std::string_view TabStateName(ShellTabNavigationState state)
{
  switch (state)
  {
  case ShellTabNavigationState::kEmpty:
    return "empty";
  case ShellTabNavigationState::kLoading:
    return "loading";
  case ShellTabNavigationState::kCommitted:
    return "committed";
  case ShellTabNavigationState::kBlocked:
    return "blocked";
  case ShellTabNavigationState::kFailed:
    return "failed";
  case ShellTabNavigationState::kCrashed:
    return "crashed";
  }

  return "unknown";
}

[[nodiscard]] const ShellTabSnapshot* FindActiveTab(const BrowserShellSnapshot& snapshot)
{
  if (!snapshot.active_tab)
  {
    return nullptr;
  }

  for (const ShellTabSnapshot& tab : snapshot.tabs)
  {
    if (tab.id == snapshot.active_tab)
    {
      return &tab;
    }
  }

  return nullptr;
}

[[nodiscard]] std::string_view DisplayUrl(const ShellTabSnapshot& tab)
{
  if (!tab.current_url.empty())
  {
    return tab.current_url;
  }

  if (!tab.pending_url.empty())
  {
    return tab.pending_url;
  }

  return "(empty)";
}

} // namespace

BrowserShellDelegate::~BrowserShellDelegate() = default;

BrowserShell::BrowserShell() = default;

BrowserShell::BrowserShell(BrowserShellDelegate& delegate,
                           std::istream& input,
                           std::ostream& output)
    : delegate_(&delegate),
      input_(&input),
      output_(&output)
{}

void BrowserShell::Show() const
{
  if (output_ == nullptr)
  {
    base::Log(base::LogLevel::kInfo, "ui", "browser shell stub ready");
    return;
  }

  *output_ << "Speed browser shell\n";
  *output_ << "Type 'help' for commands.\n";
}

int BrowserShell::Run()
{
  if (delegate_ == nullptr || input_ == nullptr || output_ == nullptr)
  {
    base::Log(base::LogLevel::kError, "ui", "browser shell delegate is not attached");
    return 1;
  }

  Show();
  PrintTabs();

  std::string line;
  while (true)
  {
    *output_ << "speed> ";
    output_->flush();

    if (!std::getline(*input_, line))
    {
      const base::Status shutdown_status = delegate_->Shutdown();
      PrintStatus(shutdown_status);
      return shutdown_status.ok() ? 0 : 1;
    }

    if (!RunCommand(line))
    {
      return 0;
    }
  }
}

base::Status BrowserShell::RunSmokeNavigation(std::string_view url)
{
  if (delegate_ == nullptr || output_ == nullptr)
  {
    return base::Status::Error("browser shell delegate is not attached");
  }

  const base::Status navigation_status = delegate_->NavigateActiveTab(url);
  PrintStatus(navigation_status);
  if (navigation_status.ok())
  {
    PrintActivePage();
  }
  else
  {
    PrintActiveError();
  }

  const base::Status shutdown_status = delegate_->Shutdown();
  if (!shutdown_status.ok())
  {
    return shutdown_status;
  }

  return navigation_status;
}

bool BrowserShell::RunCommand(std::string_view line)
{
  const std::string command = ToAsciiLower(FirstToken(line));
  const std::string_view argument = RemainingInput(line);

  if (command.empty())
  {
    return true;
  }

  if (command == "help" || command == "?")
  {
    PrintHelp();
    return true;
  }

  if (command == "open" || command == "go" || command == "nav" || command == "navigate")
  {
    return RunNavigationCommand(argument);
  }

  if (command == "new")
  {
    return RunNewTabCommand(argument);
  }

  if (command == "switch")
  {
    return RunSwitchCommand(argument);
  }

  if (command == "close")
  {
    return RunCloseCommand(argument);
  }

  if (command == "tabs" || command == "list")
  {
    PrintTabs();
    return true;
  }

  if (command == "history")
  {
    PrintHistory();
    return true;
  }

  if (command == "page")
  {
    PrintActivePage();
    return true;
  }

  if (command == "error")
  {
    PrintActiveError();
    return true;
  }

  if (command == "quit" || command == "exit")
  {
    const base::Status shutdown_status = delegate_->Shutdown();
    PrintStatus(shutdown_status);
    return false;
  }

  *output_ << "unknown command: " << command << '\n';
  return true;
}

bool BrowserShell::RunNavigationCommand(std::string_view url)
{
  if (url.empty())
  {
    *output_ << "usage: open <url>\n";
    return true;
  }

  const base::Status status = delegate_->NavigateActiveTab(url);
  PrintStatus(status);
  if (status.ok())
  {
    PrintActivePage();
  }
  else
  {
    PrintActiveError();
  }
  return true;
}

bool BrowserShell::RunNewTabCommand(std::string_view url)
{
  base::TabId created_tab;
  const base::Status create_status = delegate_->CreateTab(created_tab);
  if (!create_status.ok())
  {
    PrintStatus(create_status);
    return true;
  }

  *output_ << "created tab " << created_tab.value() << '\n';
  if (!url.empty())
  {
    return RunNavigationCommand(url);
  }

  PrintTabs();
  return true;
}

bool BrowserShell::RunSwitchCommand(std::string_view tab_id)
{
  const std::optional<base::TabId> parsed_tab_id = ParseTabId(tab_id);
  if (!parsed_tab_id.has_value())
  {
    *output_ << "usage: switch <tab-id>\n";
    return true;
  }

  const base::Status status = delegate_->SwitchToTab(*parsed_tab_id);
  PrintStatus(status);
  PrintTabs();
  return true;
}

bool BrowserShell::RunCloseCommand(std::string_view tab_id)
{
  base::TabId target_tab;
  if (tab_id.empty())
  {
    target_tab = delegate_->Snapshot().active_tab;
  }
  else
  {
    const std::optional<base::TabId> parsed_tab_id = ParseTabId(tab_id);
    if (!parsed_tab_id.has_value())
    {
      *output_ << "usage: close [tab-id]\n";
      return true;
    }
    target_tab = *parsed_tab_id;
  }

  if (!target_tab)
  {
    *output_ << "no active tab to close\n";
    return true;
  }

  const base::Status status = delegate_->CloseTab(target_tab);
  PrintStatus(status);
  PrintTabs();
  return true;
}

void BrowserShell::PrintHelp() const
{
  *output_ << "commands:\n";
  *output_ << "  open <url>       navigate the active tab\n";
  *output_ << "  new [url]        create a tab, optionally navigate it\n";
  *output_ << "  switch <tab-id>  switch active tab\n";
  *output_ << "  close [tab-id]   close a tab\n";
  *output_ << "  tabs             list tabs\n";
  *output_ << "  history          show visited URLs\n";
  *output_ << "  page             show active page content\n";
  *output_ << "  error            show active tab error\n";
  *output_ << "  quit             shut down\n";
}

void BrowserShell::PrintTabs() const
{
  const BrowserShellSnapshot snapshot = delegate_->Snapshot();
  if (snapshot.tabs.empty())
  {
    *output_ << "tabs: none\n";
    return;
  }

  for (const ShellTabSnapshot& tab : snapshot.tabs)
  {
    const char active_marker = tab.id == snapshot.active_tab ? '*' : ' ';
    *output_ << active_marker << " tab " << tab.id.value() << " ["
             << TabStateName(tab.navigation_state) << "] " << DisplayUrl(tab);
    if (!tab.last_error.empty())
    {
      *output_ << " error=\"" << tab.last_error << '"';
    }
    *output_ << '\n';
  }
}

void BrowserShell::PrintHistory() const
{
  const BrowserShellSnapshot snapshot = delegate_->Snapshot();
  if (snapshot.history.empty())
  {
    *output_ << "history: empty\n";
    return;
  }

  std::size_t index = 1;
  for (const std::string& url : snapshot.history)
  {
    *output_ << index << ". " << url << '\n';
    ++index;
  }
}

void BrowserShell::PrintActivePage() const
{
  const BrowserShellSnapshot snapshot = delegate_->Snapshot();
  if (!snapshot.active_tab)
  {
    *output_ << "page: no active tab\n";
    return;
  }

  if (!snapshot.active_page.has_value())
  {
    const ShellTabSnapshot* const active_tab = FindActiveTab(snapshot);
    if (active_tab != nullptr && !active_tab->last_error.empty())
    {
      *output_ << "page error: " << active_tab->last_error << '\n';
      return;
    }

    *output_ << "page: no committed document for active tab\n";
    return;
  }

  const ShellPageSnapshot& page = *snapshot.active_page;
  *output_ << (page.is_error_page ? "error page tab " : "page tab ") << page.tab_id.value()
           << " document " << page.document_id.value() << '\n';
  *output_ << "url: " << page.url << '\n';
  *output_ << page.document_body << '\n';
}

void BrowserShell::PrintActiveError() const
{
  const BrowserShellSnapshot snapshot = delegate_->Snapshot();
  const ShellTabSnapshot* const active_tab = FindActiveTab(snapshot);
  if (active_tab == nullptr)
  {
    *output_ << "error: no active tab\n";
    return;
  }

  if (active_tab->last_error.empty())
  {
    *output_ << "error: none\n";
    return;
  }

  *output_ << "error: " << active_tab->last_error << '\n';
}

void BrowserShell::PrintStatus(const base::Status& status) const
{
  if (status.ok())
  {
    *output_ << "ok\n";
    return;
  }

  *output_ << "error: " << status.message() << '\n';
}

} // namespace speed::ui
