#include "renderer/document/renderer_ipc_server.h"

#include "ipc/runtime/navigation_codec.h"

#include <optional>
#include <string>
#include <utility>

namespace speed::renderer
{

namespace
{

[[nodiscard]] bool IsClosedTransportStatus(const base::Status& status)
{
  return status.message().find("IPC transport closed") != std::string::npos;
}

[[nodiscard]] ipc::navigation::RenderCommandType
MapCommandType(engine::paint::DisplayCommandType type)
{
  switch (type)
  {
  case engine::paint::DisplayCommandType::kRect:
    return ipc::navigation::RenderCommandType::kRect;
  case engine::paint::DisplayCommandType::kText:
    return ipc::navigation::RenderCommandType::kText;
  case engine::paint::DisplayCommandType::kBorder:
    return ipc::navigation::RenderCommandType::kBorder;
  case engine::paint::DisplayCommandType::kImagePlaceholder:
    return ipc::navigation::RenderCommandType::kImagePlaceholder;
  }

  return ipc::navigation::RenderCommandType::kRect;
}

[[nodiscard]] ipc::navigation::RenderDisplayCommand
MapDisplayCommand(const engine::paint::DisplayCommand& command)
{
  return {
      .type = MapCommandType(command.type),
      .x = command.rect.x,
      .y = command.rect.y,
      .width = command.rect.width,
      .height = command.rect.height,
      .color_red = command.color.red,
      .color_green = command.color.green,
      .color_blue = command.color.blue,
      .color_alpha = command.color.alpha,
      .border_top = command.border_width.top,
      .border_right = command.border_width.right,
      .border_bottom = command.border_width.bottom,
      .border_left = command.border_width.left,
      .font_size_px = command.font_size_px,
      .text = command.text,
  };
}

[[nodiscard]] ipc::navigation::RenderReady
BuildRenderReady(const CommittedDocument& document, base::Status render_status = base::Status::Ok())
{
  ipc::navigation::RenderReady ready{
      .tab_id = document.tab_id,
      .document_id = document.document_id,
      .ok = render_status.ok(),
      .is_error_page = document.is_error_page,
      .content_height = document.render_result.layout.content_height,
      .error_message = render_status.ok() ? std::string() : render_status.message(),
      .display_commands = {},
  };
  ready.display_commands.reserve(document.render_result.display_list.commands.size());
  for (const engine::paint::DisplayCommand& command : document.render_result.display_list.commands)
  {
    ready.display_commands.push_back(MapDisplayCommand(command));
  }
  return ready;
}

[[nodiscard]] ipc::navigation::RenderReady
BuildFailureRenderReady(base::TabId tab_id, base::DocumentId document_id, std::string message)
{
  return {
      .tab_id = tab_id,
      .document_id = document_id,
      .ok = false,
      .is_error_page = true,
      .content_height = 0,
      .error_message = std::move(message),
      .display_commands = {},
  };
}

} // namespace

RendererIpcServer::RendererIpcServer(RendererProcess& process,
                                     ipc::FileDescriptorTransport transport)
    : process_(process),
      transport_(std::move(transport))
{}

base::Status RendererIpcServer::RunOnce()
{
  ipc::Message message({}, {});
  base::Status status = transport_.ReceiveMessage(message);
  if (!status.ok())
  {
    return status;
  }

  ipc::navigation::RenderReady ready;
  if (message.header().message_name == ipc::navigation::kCommitDocumentMessageName)
  {
    ipc::navigation::CommitDocument commit;
    status = ipc::navigation::DecodeCommitDocument(message, commit);
    if (!status.ok())
    {
      return status;
    }

    status = process_.CommitDocument(commit);
    const std::optional<CommittedDocument> document = process_.LastCommittedDocument();
    ready = status.ok() && document.has_value()
                ? BuildRenderReady(*document)
                : BuildFailureRenderReady(commit.tab_id, commit.document_id, status.message());
  }
  else if (message.header().message_name == ipc::navigation::kCommitErrorPageMessageName)
  {
    ipc::navigation::CommitErrorPage commit;
    status = ipc::navigation::DecodeCommitErrorPage(message, commit);
    if (!status.ok())
    {
      return status;
    }

    status = process_.CommitErrorPage(commit);
    const std::optional<CommittedDocument> document = process_.LastCommittedDocument();
    ready = status.ok() && document.has_value()
                ? BuildRenderReady(*document)
                : BuildFailureRenderReady(commit.tab_id, commit.document_id, status.message());
  }
  else
  {
    return base::Status::Error("renderer IPC message is not a document commit");
  }

  return transport_.SendMessage(ipc::navigation::EncodeRenderReady(ready));
}

base::Status RendererIpcServer::RunForCommitCount(int commit_count)
{
  for (int index = 0; index < commit_count; ++index)
  {
    const base::Status status = RunOnce();
    if (!status.ok())
    {
      return status;
    }
  }

  return base::Status::Ok();
}

base::Status RendererIpcServer::RunUntilClosed()
{
  for (;;)
  {
    const base::Status status = RunOnce();
    if (!status.ok())
    {
      return IsClosedTransportStatus(status) ? base::Status::Ok() : status;
    }
  }
}

} // namespace speed::renderer
