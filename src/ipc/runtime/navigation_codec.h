#pragma once

#include "base/result/status.h"
#include "ipc/runtime/message.h"
#include "ipc/runtime/navigation_messages.h"

#include <string_view>

namespace speed::ipc::navigation
{

inline constexpr std::string_view kNavigateRequestMessageName = "NavigateRequest";
inline constexpr std::string_view kNavigateResponseMessageName = "NavigateResponse";
inline constexpr std::string_view kCommitDocumentMessageName = "CommitDocument";
inline constexpr std::string_view kCommitErrorPageMessageName = "CommitErrorPage";
inline constexpr std::string_view kRenderReadyMessageName = "RenderReady";

[[nodiscard]] Message EncodeNavigateRequest(const NavigateRequest& request);
[[nodiscard]] Message EncodeNavigateResponse(const NavigateResponse& response);
[[nodiscard]] Message EncodeCommitDocument(const CommitDocument& commit);
[[nodiscard]] Message EncodeCommitErrorPage(const CommitErrorPage& commit);
[[nodiscard]] Message EncodeRenderReady(const RenderReady& ready);

[[nodiscard]] base::Status DecodeNavigateRequest(const Message& message, NavigateRequest& request);
[[nodiscard]] base::Status DecodeNavigateResponse(const Message& message,
                                                  NavigateResponse& response);
[[nodiscard]] base::Status DecodeCommitDocument(const Message& message, CommitDocument& commit);
[[nodiscard]] base::Status DecodeCommitErrorPage(const Message& message, CommitErrorPage& commit);
[[nodiscard]] base::Status DecodeRenderReady(const Message& message, RenderReady& ready);

} // namespace speed::ipc::navigation
