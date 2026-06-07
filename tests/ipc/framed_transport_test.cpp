#include "ipc/runtime/codec.h"
#include "ipc/runtime/framed_transport.h"
#include "ipc/runtime/navigation_codec.h"

#include <cassert>
#include <string>

namespace
{

[[nodiscard]] speed::ipc::Message EmptyMessage()
{
  return speed::ipc::Message({}, {});
}

void DecodesNavigationMessages()
{
  const speed::ipc::navigation::NavigateRequest request{
      .request_id = speed::base::RequestId::FromRaw(7),
      .tab_id = speed::base::TabId::FromRaw(3),
      .url = "https://example.test/page",
      .is_top_level = true,
  };
  speed::ipc::Message message = speed::ipc::navigation::EncodeNavigateRequest(request);
  speed::ipc::Message decoded = EmptyMessage();
  speed::base::Status status =
      speed::ipc::DecodeMessage(speed::ipc::EncodeMessage(message), decoded);
  assert(status.ok());
  speed::ipc::navigation::NavigateRequest decoded_request;
  status = speed::ipc::navigation::DecodeNavigateRequest(decoded, decoded_request);
  assert(status.ok());
  assert(decoded_request.request_id == request.request_id);
  assert(decoded_request.tab_id == request.tab_id);
  assert(decoded_request.url == request.url);
  assert(decoded_request.is_top_level);

  const speed::ipc::navigation::NavigateResponse response{
      .request_id = request.request_id,
      .status = speed::ipc::navigation::NavigateStatus::kBlocked,
      .aegis_reason = "blocked by test",
      .error_message = {},
      .document_body = {},
  };
  message = speed::ipc::navigation::EncodeNavigateResponse(response);
  status = speed::ipc::DecodeMessage(speed::ipc::EncodeMessage(message), decoded);
  assert(status.ok());
  speed::ipc::navigation::NavigateResponse decoded_response;
  status = speed::ipc::navigation::DecodeNavigateResponse(decoded, decoded_response);
  assert(status.ok());
  assert(decoded_response.request_id == response.request_id);
  assert(decoded_response.status == response.status);
  assert(decoded_response.aegis_reason == response.aegis_reason);

  const speed::ipc::navigation::CommitDocument commit{
      .tab_id = request.tab_id,
      .document_id = speed::base::DocumentId::FromRaw(9),
      .url = request.url,
      .document_body = "<html><body>Speed</body></html>",
  };
  message = speed::ipc::navigation::EncodeCommitDocument(commit);
  status = speed::ipc::DecodeMessage(speed::ipc::EncodeMessage(message), decoded);
  assert(status.ok());
  speed::ipc::navigation::CommitDocument decoded_commit;
  status = speed::ipc::navigation::DecodeCommitDocument(decoded, decoded_commit);
  assert(status.ok());
  assert(decoded_commit.document_id == commit.document_id);
  assert(decoded_commit.document_body == commit.document_body);

  const speed::ipc::navigation::CommitErrorPage error_page{
      .tab_id = request.tab_id,
      .document_id = speed::base::DocumentId::FromRaw(10),
      .url = "https://ads.example/tracker",
      .reason = speed::ipc::navigation::ErrorPageReason::kBlocked,
      .message = "blocked",
  };
  message = speed::ipc::navigation::EncodeCommitErrorPage(error_page);
  status = speed::ipc::DecodeMessage(speed::ipc::EncodeMessage(message), decoded);
  assert(status.ok());
  speed::ipc::navigation::CommitErrorPage decoded_error_page;
  status = speed::ipc::navigation::DecodeCommitErrorPage(decoded, decoded_error_page);
  assert(status.ok());
  assert(decoded_error_page.reason == error_page.reason);
  assert(decoded_error_page.message == error_page.message);

  const speed::ipc::navigation::RenderReady ready{
      .tab_id = request.tab_id,
      .document_id = speed::base::DocumentId::FromRaw(11),
      .ok = true,
      .is_error_page = false,
      .content_height = 48,
      .error_message = {},
      .display_commands =
          {
              {
                  .type = speed::ipc::navigation::RenderCommandType::kText,
                  .x = 2,
                  .y = 4,
                  .width = 40,
                  .height = 18,
                  .color_red = 1,
                  .color_green = 2,
                  .color_blue = 3,
                  .color_alpha = 255,
                  .font_size_px = 18,
                  .text = "Ready",
              },
          },
  };
  message = speed::ipc::navigation::EncodeRenderReady(ready);
  status = speed::ipc::DecodeMessage(speed::ipc::EncodeMessage(message), decoded);
  assert(status.ok());
  speed::ipc::navigation::RenderReady decoded_ready;
  status = speed::ipc::navigation::DecodeRenderReady(decoded, decoded_ready);
  assert(status.ok());
  assert(decoded_ready.document_id == ready.document_id);
  assert(decoded_ready.content_height == ready.content_height);
  assert(decoded_ready.display_commands.size() == 1);
  assert(decoded_ready.display_commands.front().text == "Ready");
}

void RejectsMalformedFramesAndPayloads()
{
  speed::ipc::Message decoded = EmptyMessage();
  speed::base::Status status = speed::ipc::DecodeMessage({}, decoded);
  assert(!status.ok());

  status = speed::ipc::DecodeMessage("BAD!", decoded);
  assert(!status.ok());

  const speed::ipc::Message unknown_schema(
      {
          .sender = speed::ipc::ProcessRole::kBrowser,
          .receiver = speed::ipc::ProcessRole::kNetwork,
          .schema_name = "speed.unknown.v0",
          .schema_version = 1,
          .message_name = "NavigateRequest",
      },
      "payload");
  status = speed::ipc::DecodeMessage(speed::ipc::EncodeMessage(unknown_schema), decoded);
  assert(!status.ok());

  const speed::ipc::Message malformed_payload(
      {
          .sender = speed::ipc::ProcessRole::kBrowser,
          .receiver = speed::ipc::ProcessRole::kNetwork,
          .schema_name = std::string(speed::ipc::navigation::kSchemaName),
          .schema_version = speed::ipc::navigation::kSchemaVersion,
          .message_name = std::string(speed::ipc::navigation::kNavigateRequestMessageName),
      },
      "x");
  speed::ipc::navigation::NavigateRequest request;
  status = speed::ipc::navigation::DecodeNavigateRequest(malformed_payload, request);
  assert(!status.ok());
}

void RoundTripsOverLocalTransport()
{
  speed::ipc::LocalTransportPair pair;
  speed::base::Status status = speed::ipc::CreateLocalTransportPair(pair);
  assert(status.ok());

  const speed::ipc::navigation::NavigateRequest request{
      .request_id = speed::base::RequestId::FromRaw(11),
      .tab_id = speed::base::TabId::FromRaw(4),
      .url = "about:blank",
      .is_top_level = true,
  };
  status = pair.first.SendMessage(speed::ipc::navigation::EncodeNavigateRequest(request));
  assert(status.ok());

  speed::ipc::Message received = EmptyMessage();
  status = pair.second.ReceiveMessage(received);
  assert(status.ok());

  speed::ipc::navigation::NavigateRequest decoded_request;
  status = speed::ipc::navigation::DecodeNavigateRequest(received, decoded_request);
  assert(status.ok());
  assert(decoded_request.request_id == request.request_id);
  assert(decoded_request.url == request.url);
}

} // namespace

int main()
{
  DecodesNavigationMessages();
  RejectsMalformedFramesAndPayloads();
  RoundTripsOverLocalTransport();
  return 0;
}
