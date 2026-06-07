#pragma once

#include "base/ids/id_types.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

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

enum class RenderCommandType : std::uint8_t
{
  kRect,
  kText,
  kBorder,
  kImagePlaceholder,
};

struct RenderDisplayCommand final
{
  RenderCommandType type{RenderCommandType::kRect};
  std::int32_t x{0};
  std::int32_t y{0};
  std::int32_t width{0};
  std::int32_t height{0};
  std::uint8_t color_red{0};
  std::uint8_t color_green{0};
  std::uint8_t color_blue{0};
  std::uint8_t color_alpha{255};
  std::int32_t border_top{0};
  std::int32_t border_right{0};
  std::int32_t border_bottom{0};
  std::int32_t border_left{0};
  std::int32_t font_size_px{16};
  std::string text;
};

struct RenderReady final
{
  base::TabId tab_id;
  base::DocumentId document_id;
  bool ok{false};
  bool is_error_page{false};
  std::int32_t content_height{0};
  std::string error_message;
  std::vector<RenderDisplayCommand> display_commands;
};

[[nodiscard]] bool IsValidNavigateRequest(const NavigateRequest& request);
[[nodiscard]] bool IsValidNavigateResponse(const NavigateResponse& response);
[[nodiscard]] bool IsValidCommitDocument(const CommitDocument& commit);
[[nodiscard]] bool IsValidCommitErrorPage(const CommitErrorPage& commit);
[[nodiscard]] bool IsValidRenderReady(const RenderReady& ready);
[[nodiscard]] std::string_view NavigateStatusName(NavigateStatus status);
[[nodiscard]] std::string_view ErrorPageReasonName(ErrorPageReason reason);
[[nodiscard]] std::string_view RenderCommandTypeName(RenderCommandType type);

} // namespace speed::ipc::navigation
