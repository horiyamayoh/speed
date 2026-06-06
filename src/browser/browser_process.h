#pragma once

#include "base/ids/id_types.h"
#include "base/result/status.h"
#include "browser/navigation/navigation_controller.h"
#include "browser/tabs/tab_model.h"
#include "storage/history/history_store.h"

#include <cstdint>
#include <string_view>

namespace speed::browser
{

class BrowserProcess final
{
public:
  enum class State : std::uint8_t
  {
    kCreated,
    kRunning,
    kStopped,
  };

  [[nodiscard]] base::Status Start();
  [[nodiscard]] base::Status Shutdown();

  [[nodiscard]] base::Status NavigateActiveTab(std::string_view input);
  [[nodiscard]] base::Status NavigateTab(base::TabId tab_id, std::string_view input);

  [[nodiscard]] State state() const;
  [[nodiscard]] bool running() const;
  [[nodiscard]] const TabModel& tabs() const;
  [[nodiscard]] const storage::HistoryStore& history() const;

private:
  TabModel tabs_;
  NavigationController navigation_;
  storage::HistoryStore history_;
  State state_{State::kCreated};
};

int RunBrowserProcess();

} // namespace speed::browser
