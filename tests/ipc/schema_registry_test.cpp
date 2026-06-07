#include "ipc/runtime/message.h"
#include "ipc/runtime/navigation_messages.h"

#include <cassert>
#include <filesystem>

int main()
{
  assert(speed::ipc::IsKnownSchema("speed.navigation.v0"));
  assert(!speed::ipc::IsKnownSchema("speed.unknown.v0"));

  const std::filesystem::path schema_path =
      std::filesystem::path(SPEED_SOURCE_DIR) / "src/ipc/schemas/navigation.speedipc";
  assert(std::filesystem::exists(schema_path));
  assert(speed::ipc::navigation::kSchemaName == "speed.navigation.v0");
  assert(speed::ipc::navigation::kSchemaVersion == 1);
  assert(speed::ipc::navigation::IsValidNavigateRequest({
      .request_id = speed::base::RequestId::FromRaw(1),
      .tab_id = speed::base::TabId::FromRaw(2),
      .url = "https://example.test",
      .is_top_level = true,
  }));
  assert(speed::ipc::navigation::IsValidNavigateResponse({
      .request_id = speed::base::RequestId::FromRaw(1),
      .status = speed::ipc::navigation::NavigateStatus::kAllowed,
      .aegis_reason = "no matching Aegis rule",
      .error_message = {},
      .document_body = "<html><body></body></html>",
  }));
  assert(speed::ipc::navigation::IsValidCommitDocument({
      .tab_id = speed::base::TabId::FromRaw(2),
      .document_id = speed::base::DocumentId::FromRaw(3),
      .url = "https://example.test",
      .document_body = "<html><body></body></html>",
  }));
  assert(speed::ipc::navigation::IsValidCommitErrorPage({
      .tab_id = speed::base::TabId::FromRaw(2),
      .document_id = speed::base::DocumentId::FromRaw(4),
      .url = "https://ads.example",
      .reason = speed::ipc::navigation::ErrorPageReason::kBlocked,
      .message = "blocked by test",
  }));

  return 0;
}
