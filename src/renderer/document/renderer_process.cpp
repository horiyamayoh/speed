#include "renderer/document/renderer_process.h"

#include "base/logging/logging.h"
#include "engine/html/html_parser.h"

#include <utility>

namespace speed::renderer
{

base::Status RendererProcess::CommitDocument(ipc::navigation::CommitDocument commit)
{
  if (!ipc::navigation::IsValidCommitDocument(commit))
  {
    return base::Status::Error("invalid document commit");
  }

  const std::string body_ref = commit.body_ref;
  const engine::html::HtmlParser parser;
  committed_documents_.push_back({
      .commit = std::move(commit),
      .document = parser.ParseFragment(body_ref),
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

int RunRendererProcess()
{
  RendererProcess process;
  const base::Status commit_status = process.CommitDocument({
      .tab_id = base::TabId::FromRaw(1),
      .document_id = base::DocumentId::FromRaw(1),
      .url = "about:blank",
      .body_ref = "stub-document:about:blank",
  });
  (void)commit_status;

  base::Log(base::LogLevel::kInfo, "renderer", "renderer process stub ready");
  return 0;
}

} // namespace speed::renderer
