#include "renderer/document/renderer_process.h"

#include <cassert>
#include <optional>
#include <string>

int main()
{
  speed::renderer::RendererProcess renderer;
  assert(renderer.committed_document_count() == 0);

  speed::base::Status status = renderer.CommitDocument({
      .tab_id = speed::base::TabId::FromRaw(1),
      .document_id = speed::base::DocumentId::FromRaw(2),
      .url = "https://example.test",
      .document_body =
          "<html><head><style>p { color: red; }</style></head><body><p>Speed</p></body></html>",
  });
  assert(status.ok());
  assert(renderer.committed_document_count() == 1);

  const std::optional<speed::renderer::CommittedDocument> document =
      renderer.LastCommittedDocument();
  assert(document.has_value());
  assert(document->url == "https://example.test");
  assert(document->document_body.find("<p>Speed</p>") != std::string::npos);
  assert(!document->is_error_page);
  assert(document->document.children.size() == 1);
  assert(document->document.children.front().name == "html");
  assert(!document->render_result.display_list.commands.empty());

  status = renderer.CommitErrorPage({
      .tab_id = speed::base::TabId::FromRaw(1),
      .document_id = speed::base::DocumentId::FromRaw(3),
      .url = "https://ads.example",
      .reason = speed::ipc::navigation::ErrorPageReason::kBlocked,
      .message = "blocked <by test>",
  });
  assert(status.ok());
  assert(renderer.committed_document_count() == 2);
  const std::optional<speed::renderer::CommittedDocument> error_document =
      renderer.LastCommittedDocument();
  assert(error_document.has_value());
  assert(error_document->is_error_page);
  assert(error_document->error_reason == speed::ipc::navigation::ErrorPageReason::kBlocked);
  assert(error_document->document_body.find("&lt;by test&gt;") != std::string::npos);

  status = renderer.CommitDocument({
      .tab_id = speed::base::TabId::FromRaw(1),
      .document_id = {},
      .url = "https://example.test",
      .document_body = "<html><body></body></html>",
  });
  assert(!status.ok());
  assert(renderer.committed_document_count() == 2);

  return 0;
}
