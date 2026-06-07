#include "renderer/document/renderer_process.h"

#include "base/logging/logging.h"
#include "engine/html/html_parser.h"
#include "engine/render_pipeline.h"
#include "renderer/document/renderer_ipc_server.h"

#include <string>
#include <string_view>
#include <utility>

namespace speed::renderer
{

namespace
{

[[nodiscard]] std::string EscapeHtml(std::string_view input)
{
  std::string output;
  output.reserve(input.size());
  for (const char character : input)
  {
    switch (character)
    {
    case '&':
      output += "&amp;";
      break;
    case '<':
      output += "&lt;";
      break;
    case '>':
      output += "&gt;";
      break;
    case '"':
      output += "&quot;";
      break;
    case '\'':
      output += "&#39;";
      break;
    default:
      output += character;
      break;
    }
  }
  return output;
}

[[nodiscard]] std::string ErrorTitle(ipc::navigation::ErrorPageReason reason)
{
  switch (reason)
  {
  case ipc::navigation::ErrorPageReason::kBlocked:
    return "Blocked by Aegis";
  case ipc::navigation::ErrorPageReason::kFailed:
    return "Navigation failed";
  case ipc::navigation::ErrorPageReason::kCrashed:
    return "Tab crashed";
  }

  return "Navigation error";
}

[[nodiscard]] std::string BuildErrorPageBody(const ipc::navigation::CommitErrorPage& commit)
{
  const std::string title = ErrorTitle(commit.reason);
  return "<html><body><h1>" + title + "</h1><p>" + EscapeHtml(commit.message) + "</p><p>" +
         EscapeHtml(commit.url) + "</p></body></html>";
}

} // namespace

base::Status RendererProcess::CommitDocument(ipc::navigation::CommitDocument commit)
{
  if (!ipc::navigation::IsValidCommitDocument(commit))
  {
    return base::Status::Error("invalid document commit");
  }

  const std::string document_body = commit.document_body;
  const engine::html::HtmlParser parser;
  const engine::dom::Node document = parser.ParseFragment(document_body);
  const engine::RenderPipeline render_pipeline;
  committed_documents_.push_back({
      .tab_id = commit.tab_id,
      .document_id = commit.document_id,
      .url = std::move(commit.url),
      .document_body = document_body,
      .is_error_page = false,
      .error_reason = ipc::navigation::ErrorPageReason::kFailed,
      .document = document,
      .render_result = render_pipeline.RenderDocument(document, {.width = 800, .height = 600}),
  });
  return base::Status::Ok();
}

base::Status RendererProcess::CommitErrorPage(ipc::navigation::CommitErrorPage commit)
{
  if (!ipc::navigation::IsValidCommitErrorPage(commit))
  {
    return base::Status::Error("invalid error page commit");
  }

  const std::string document_body = BuildErrorPageBody(commit);
  const engine::html::HtmlParser parser;
  const engine::dom::Node document = parser.ParseFragment(document_body);
  const engine::RenderPipeline render_pipeline;
  committed_documents_.push_back({
      .tab_id = commit.tab_id,
      .document_id = commit.document_id,
      .url = std::move(commit.url),
      .document_body = document_body,
      .is_error_page = true,
      .error_reason = commit.reason,
      .document = document,
      .render_result = render_pipeline.RenderDocument(document, {.width = 800, .height = 600}),
  });
  return base::Status::Ok();
}

std::size_t RendererProcess::committed_document_count() const
{
  return committed_documents_.size();
}

std::optional<CommittedDocument> RendererProcess::LastCommittedDocument() const
{
  if (committed_documents_.empty())
  {
    return std::nullopt;
  }

  return committed_documents_.back();
}

std::vector<CommittedDocument> RendererProcess::committed_documents() const
{
  return committed_documents_;
}

int RunRendererProcess(int ipc_fd, int crash_after_commit_count)
{
  if (ipc_fd >= 0)
  {
    RendererProcess process;
    RendererIpcServer server(process, ipc::FileDescriptorTransport(ipc_fd));
    const base::Status status = crash_after_commit_count > 0
                                    ? server.RunForCommitCount(crash_after_commit_count)
                                    : server.RunUntilClosed();
    if (!status.ok())
    {
      base::Log(base::LogLevel::kError, "renderer", status.message());
      return 1;
    }

    return crash_after_commit_count > 0 ? 70 : 0;
  }

  RendererProcess process;
  const base::Status commit_status = process.CommitDocument({
      .tab_id = base::TabId::FromRaw(1),
      .document_id = base::DocumentId::FromRaw(1),
      .url = "about:blank",
      .document_body = "<html><body></body></html>",
  });
  (void)commit_status;

  base::Log(base::LogLevel::kInfo, "renderer", "renderer process stub ready");
  return 0;
}

} // namespace speed::renderer
