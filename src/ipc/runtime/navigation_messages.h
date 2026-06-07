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
  std::string document_body;
};

struct CommitDocument final
{
  base::TabId tab_id;
  base::DocumentId document_id;
  std::string url;
  std::string document_body;
};

enum class ErrorPageReason : std::uint8_t
{
  kBlocked,
  kFailed,
  kCrashed,
};

struct CommitErrorPage final
{
  base::TabId tab_id;
  base::DocumentId document_id;
  std::string url;
  ErrorPageReason reason{ErrorPageReason::kFailed};
  std::string message;
};

[[nodiscard]] bool IsValidNavigateRequest(const NavigateRequest& request);
[[nodiscard]] bool IsValidNavigateResponse(const NavigateResponse& response);
[[nodiscard]] bool IsValidCommitDocument(const CommitDocument& commit);
[[nodiscard]] bool IsValidCommitErrorPage(const CommitErrorPage& commit);
[[nodiscard]] std::string_view NavigateStatusName(NavigateStatus status);
[[nodiscard]] std::string_view ErrorPageReasonName(ErrorPageReason reason);

} // namespace speed::ipc::navigation
