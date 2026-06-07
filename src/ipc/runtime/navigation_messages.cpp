#include "ipc/runtime/navigation_messages.h"

namespace speed::ipc::navigation
{

bool IsValidNavigateRequest(const NavigateRequest& request)
{
  return request.request_id && request.tab_id && request.is_top_level && !request.url.empty();
}

bool IsValidNavigateResponse(const NavigateResponse& response)
{
  if (!response.request_id)
  {
    return false;
  }

  switch (response.status)
  {
  case NavigateStatus::kAllowed:
    return response.error_message.empty() && !response.document_body.empty();
  case NavigateStatus::kBlocked:
    return !response.aegis_reason.empty() && response.document_body.empty();
  case NavigateStatus::kFailed:
    return !response.error_message.empty() && response.document_body.empty();
  }

  return false;
}

bool IsValidCommitDocument(const CommitDocument& commit)
{
  return commit.tab_id && commit.document_id && !commit.url.empty() &&
         !commit.document_body.empty();
}

bool IsValidCommitErrorPage(const CommitErrorPage& commit)
{
  return commit.tab_id && commit.document_id && !commit.url.empty() && !commit.message.empty();
}

bool IsValidRenderReady(const RenderReady& ready)
{
  if (!ready.tab_id || !ready.document_id || ready.content_height < 0)
  {
    return false;
  }

  if (ready.ok && !ready.error_message.empty())
  {
    return false;
  }

  if (!ready.ok && ready.error_message.empty())
  {
    return false;
  }

  for (const RenderDisplayCommand& command : ready.display_commands)
  {
    if (command.width < 0 || command.height < 0 || command.font_size_px < 0)
    {
      return false;
    }

    switch (command.type)
    {
    case RenderCommandType::kRect:
    case RenderCommandType::kText:
    case RenderCommandType::kBorder:
    case RenderCommandType::kImagePlaceholder:
      break;
    default:
      return false;
    }
  }

  return true;
}

std::string_view NavigateStatusName(NavigateStatus status)
{
  switch (status)
  {
  case NavigateStatus::kAllowed:
    return "allowed";
  case NavigateStatus::kBlocked:
    return "blocked";
  case NavigateStatus::kFailed:
    return "failed";
  }

  return "unknown";
}

std::string_view ErrorPageReasonName(ErrorPageReason reason)
{
  switch (reason)
  {
  case ErrorPageReason::kBlocked:
    return "blocked";
  case ErrorPageReason::kFailed:
    return "failed";
  case ErrorPageReason::kCrashed:
    return "crashed";
  }

  return "unknown";
}

std::string_view RenderCommandTypeName(RenderCommandType type)
{
  switch (type)
  {
  case RenderCommandType::kRect:
    return "rect";
  case RenderCommandType::kText:
    return "text";
  case RenderCommandType::kBorder:
    return "border";
  case RenderCommandType::kImagePlaceholder:
    return "image_placeholder";
  }

  return "unknown";
}

} // namespace speed::ipc::navigation
