#pragma once

#include "base/ids/id_types.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace speed::ipc::navigation
{

inline constexpr std::string_view kSchemaName = "speed.navigation.v0";
inline constexpr std::uint32_t kSchemaVersion = 1;

enum class NavigateStatus : std::uint8_t
{
  kAllowed,
  kBlocked,
  kFailed,
};

struct NavigateRequest final
{
  base::RequestId request_id;
  base::TabId tab_id;
  std::string url;
  bool is_top_level{true};
};

struct NavigateResponse final
{
  base::RequestId request_id;
  NavigateStatus status{NavigateStatus::kFailed};
  std::string aegis_reason;
  std::string error_message;
  std::string body_ref;
};

struct CommitDocument final
{
  base::TabId tab_id;
  base::DocumentId document_id;
  std::string url;
  std::string body_ref;
};

[[nodiscard]] bool IsValidNavigateRequest(const NavigateRequest& request);
[[nodiscard]] bool IsValidNavigateResponse(const NavigateResponse& response);
[[nodiscard]] bool IsValidCommitDocument(const CommitDocument& commit);
[[nodiscard]] std::string_view NavigateStatusName(NavigateStatus status);

} // namespace speed::ipc::navigation
