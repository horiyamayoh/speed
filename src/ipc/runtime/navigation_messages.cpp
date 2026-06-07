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

} // namespace speed::ipc::navigation
