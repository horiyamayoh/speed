#pragma once

#include "base/result/status.h"
#include "engine/dom/node.h"
#include "engine/render_pipeline.h"
#include "ipc/runtime/navigation_messages.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace speed::renderer
{

struct CommittedDocument final
{
  base::TabId tab_id;
  base::DocumentId document_id;
  std::string url;
  std::string document_body;
  bool is_error_page{false};
  ipc::navigation::ErrorPageReason error_reason{ipc::navigation::ErrorPageReason::kFailed};
  engine::dom::Node document;
  engine::RenderResult render_result;
};

class RendererProcess final
{
public:
  [[nodiscard]] base::Status CommitDocument(ipc::navigation::CommitDocument commit);
  [[nodiscard]] base::Status CommitErrorPage(ipc::navigation::CommitErrorPage commit);

  [[nodiscard]] std::size_t committed_document_count() const;
  [[nodiscard]] std::optional<CommittedDocument> LastCommittedDocument() const;
  [[nodiscard]] std::vector<CommittedDocument> committed_documents() const;

private:
  std::vector<CommittedDocument> committed_documents_;
};

int RunRendererProcess(int ipc_fd = -1);

} // namespace speed::renderer
