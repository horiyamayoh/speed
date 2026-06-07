#include "renderer/document/renderer_process.h"

#include <cassert>
#include <optional>

int main()
{
  speed::renderer::RendererProcess renderer;
  assert(renderer.committed_document_count() == 0);

  speed::base::Status status = renderer.CommitDocument({
      .tab_id = speed::base::TabId::FromRaw(1),
      .document_id = speed::base::DocumentId::FromRaw(2),
      .url = "https://example.test",
      .body_ref = "stub-document:https://example.test",
  });
  assert(status.ok());
  assert(renderer.committed_document_count() == 1);

  const std::optional<speed::renderer::CommittedDocument> document =
      renderer.LastCommittedDocument();
  assert(document.has_value());
  assert(document->commit.url == "https://example.test");
  assert(document->commit.body_ref == "stub-document:https://example.test");
  assert(document->document.children.size() == 1);
  assert(document->document.children.front().text == "stub-document:https://example.test");

  status = renderer.CommitDocument({
      .tab_id = speed::base::TabId::FromRaw(1),
      .document_id = {},
      .url = "https://example.test",
      .body_ref = "stub-document:https://example.test",
  });
  assert(!status.ok());
  assert(renderer.committed_document_count() == 1);

  return 0;
}
