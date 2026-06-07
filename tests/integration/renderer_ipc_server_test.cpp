#include "ipc/runtime/framed_transport.h"
#include "ipc/runtime/navigation_codec.h"
#include "renderer/document/renderer_ipc_server.h"
#include "renderer/document/renderer_process.h"

#include <cassert>
#include <string>
#include <utility>

namespace
{

[[nodiscard]] speed::ipc::navigation::RenderReady
SendCommitThroughServer(speed::renderer::RendererIpcServer& server,
                        speed::ipc::FileDescriptorTransport& client,
                        const speed::ipc::Message& commit_message)
{
  speed::base::Status status = client.SendMessage(commit_message);
  assert(status.ok());

  status = server.RunOnce();
  assert(status.ok());

  speed::ipc::Message response_message({}, {});
  status = client.ReceiveMessage(response_message);
  assert(status.ok());

  speed::ipc::navigation::RenderReady ready;
  status = speed::ipc::navigation::DecodeRenderReady(response_message, ready);
  assert(status.ok());
  return ready;
}

[[nodiscard]] bool HasTextCommand(const speed::ipc::navigation::RenderReady& ready,
                                  const std::string& text)
{
  for (const speed::ipc::navigation::RenderDisplayCommand& command : ready.display_commands)
  {
    if (command.type == speed::ipc::navigation::RenderCommandType::kText && command.text == text)
    {
      return true;
    }
  }

  return false;
}

} // namespace

int main()
{
  speed::ipc::LocalTransportPair pair;
  speed::base::Status status = speed::ipc::CreateLocalTransportPair(pair);
  assert(status.ok());

  speed::renderer::RendererProcess process;
  speed::renderer::RendererIpcServer server(process, std::move(pair.second));

  speed::ipc::navigation::RenderReady ready = SendCommitThroughServer(
      server,
      pair.first,
      speed::ipc::navigation::EncodeCommitDocument({
          .tab_id = speed::base::TabId::FromRaw(1),
          .document_id = speed::base::DocumentId::FromRaw(2),
          .url = "https://example.test",
          .document_body =
              "<html><head><style>p { color: red; }</style></head><body><p>Speed</p></body></html>",
      }));
  assert(ready.ok);
  assert(!ready.is_error_page);
  assert(ready.content_height > 0);
  assert(!ready.display_commands.empty());
  assert(HasTextCommand(ready, "Speed"));
  assert(process.committed_document_count() == 1);

  ready = SendCommitThroughServer(server,
                                  pair.first,
                                  speed::ipc::navigation::EncodeCommitErrorPage({
                                      .tab_id = speed::base::TabId::FromRaw(1),
                                      .document_id = speed::base::DocumentId::FromRaw(3),
                                      .url = "https://ads.example",
                                      .reason = speed::ipc::navigation::ErrorPageReason::kBlocked,
                                      .message = "blocked by test",
                                  }));
  assert(ready.ok);
  assert(ready.is_error_page);
  assert(ready.content_height > 0);
  assert(!ready.display_commands.empty());
  assert(process.committed_document_count() == 2);

  return 0;
}
