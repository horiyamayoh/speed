#pragma once

#include "base/result/status.h"
#include "engine/dom/node.h"
#include "ipc/runtime/navigation_messages.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace speed::renderer
{

struct CommittedDocument final
{
  ipc::navigation::CommitDocument commit;
  engine::dom::Node document;
};

class RendererProcess final
{
public:
  [[nodiscard]] base::Status CommitDocument(ipc::navigation::CommitDocument commit);

  [[nodiscard]] std::size_t committed_document_count() const;
  [[nodiscard]] std::optional<CommittedDocument> LastCommittedDocument() const;
  [[nodiscard]] std::vector<CommittedDocument> committed_documents() const;

private:
  std::vector<CommittedDocument> committed_documents_;
};

int RunRendererProcess();

} // namespace speed::renderer
