#include "ipc/runtime/navigation_codec.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace speed::ipc::navigation
{

namespace
{

constexpr std::uint32_t kMaxRenderDisplayCommands = 100000;

class PayloadWriter final
{
public:
  void WriteU8(std::uint8_t value)
  {
    payload_.push_back(static_cast<char>(value));
  }

  void WriteBool(bool value)
  {
    WriteU8(value ? 1U : 0U);
  }

  void WriteU32(std::uint32_t value)
  {
    for (int shift = 24; shift >= 0; shift -= 8)
    {
      payload_.push_back(static_cast<char>((value >> shift) & 0xffU));
    }
  }

  void WriteI32(std::int32_t value)
  {
    WriteU32(static_cast<std::uint32_t>(value));
  }

  void WriteU64(std::uint64_t value)
  {
    for (int shift = 56; shift >= 0; shift -= 8)
    {
      payload_.push_back(static_cast<char>((value >> shift) & 0xffU));
    }
  }

  void WriteString(std::string_view value)
  {
    const std::uint32_t size =
        value.size() <= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())
            ? static_cast<std::uint32_t>(value.size())
            : 0;
    WriteU32(size);
    payload_.append(value.data(), value.size());
  }

  [[nodiscard]] std::string Finish()
  {
    return std::move(payload_);
  }

private:
  std::string payload_;
};

class PayloadReader final
{
public:
  explicit PayloadReader(std::string_view payload)
      : payload_(payload)
  {}

  [[nodiscard]] bool ReadU8(std::uint8_t& value)
  {
    if (offset_ >= payload_.size())
    {
      return false;
    }

    value = static_cast<std::uint8_t>(payload_[offset_]);
    ++offset_;
    return true;
  }

  [[nodiscard]] bool ReadBool(bool& value)
  {
    std::uint8_t raw = 0;
    if (!ReadU8(raw) || raw > 1U)
    {
      return false;
    }

    value = raw == 1U;
    return true;
  }

  [[nodiscard]] bool ReadU64(std::uint64_t& value)
  {
    if (payload_.size() - offset_ < sizeof(std::uint64_t))
    {
      return false;
    }

    value = 0;
    for (int index = 0; index < 8; ++index)
    {
      value <<= 8U;
      value |= static_cast<std::uint8_t>(payload_[offset_ + static_cast<std::size_t>(index)]);
    }
    offset_ += sizeof(std::uint64_t);
    return true;
  }

  [[nodiscard]] bool ReadU32(std::uint32_t& value)
  {
    if (payload_.size() - offset_ < sizeof(std::uint32_t))
    {
      return false;
    }

    value = 0;
    for (int index = 0; index < 4; ++index)
    {
      value <<= 8U;
      value |= static_cast<std::uint8_t>(payload_[offset_ + static_cast<std::size_t>(index)]);
    }
    offset_ += sizeof(std::uint32_t);
    return true;
  }

  [[nodiscard]] bool ReadI32(std::int32_t& value)
  {
    std::uint32_t raw = 0;
    if (!ReadU32(raw))
    {
      return false;
    }

    value = static_cast<std::int32_t>(raw);
    return true;
  }

  [[nodiscard]] bool ReadString(std::string& value)
  {
    std::uint32_t size = 0;
    if (!ReadU32(size) || payload_.size() - offset_ < size)
    {
      return false;
    }

    value = std::string(payload_.substr(offset_, size));
    offset_ += size;
    return true;
  }

  [[nodiscard]] bool AtEnd() const
  {
    return offset_ == payload_.size();
  }

private:
  std::string_view payload_;
  std::size_t offset_{0};
};

[[nodiscard]] Message MakeNavigationMessage(ProcessRole sender,
                                            ProcessRole receiver,
                                            std::string_view message_name,
                                            std::string payload)
{
  return Message(
      {
          .sender = sender,
          .receiver = receiver,
          .schema_name = std::string(kSchemaName),
          .schema_version = kSchemaVersion,
          .message_name = std::string(message_name),
      },
      std::move(payload));
}

[[nodiscard]] base::Status EnsureNavigationMessage(const Message& message,
                                                   ProcessRole expected_sender,
                                                   ProcessRole expected_receiver,
                                                   std::string_view expected_name)
{
  const MessageHeader& header = message.header();
  if (header.sender != expected_sender || header.receiver != expected_receiver)
  {
    return base::Status::Error("navigation IPC message has unexpected process roles");
  }

  if (header.schema_name != kSchemaName || header.schema_version != kSchemaVersion)
  {
    return base::Status::Error("navigation IPC message has unexpected schema");
  }

  if (header.message_name != expected_name)
  {
    return base::Status::Error("navigation IPC message has unexpected message name");
  }

  return base::Status::Ok();
}

[[nodiscard]] base::Status EnsureAtEnd(const PayloadReader& reader)
{
  if (!reader.AtEnd())
  {
    return base::Status::Error("navigation IPC payload has trailing bytes");
  }

  return base::Status::Ok();
}

void WriteRenderCommand(PayloadWriter& writer, const RenderDisplayCommand& command)
{
  writer.WriteU8(static_cast<std::uint8_t>(command.type));
  writer.WriteI32(command.x);
  writer.WriteI32(command.y);
  writer.WriteI32(command.width);
  writer.WriteI32(command.height);
  writer.WriteU8(command.color_red);
  writer.WriteU8(command.color_green);
  writer.WriteU8(command.color_blue);
  writer.WriteU8(command.color_alpha);
  writer.WriteI32(command.border_top);
  writer.WriteI32(command.border_right);
  writer.WriteI32(command.border_bottom);
  writer.WriteI32(command.border_left);
  writer.WriteI32(command.font_size_px);
  writer.WriteString(command.text);
}

[[nodiscard]] bool ReadRenderCommand(PayloadReader& reader, RenderDisplayCommand& command)
{
  std::uint8_t type = 0;
  if (!reader.ReadU8(type) ||
      type > static_cast<std::uint8_t>(RenderCommandType::kImagePlaceholder))
  {
    return false;
  }

  command.type = static_cast<RenderCommandType>(type);
  return reader.ReadI32(command.x) && reader.ReadI32(command.y) && reader.ReadI32(command.width) &&
         reader.ReadI32(command.height) && reader.ReadU8(command.color_red) &&
         reader.ReadU8(command.color_green) && reader.ReadU8(command.color_blue) &&
         reader.ReadU8(command.color_alpha) && reader.ReadI32(command.border_top) &&
         reader.ReadI32(command.border_right) && reader.ReadI32(command.border_bottom) &&
         reader.ReadI32(command.border_left) && reader.ReadI32(command.font_size_px) &&
         reader.ReadString(command.text);
}

} // namespace

Message EncodeNavigateRequest(const NavigateRequest& request)
{
  PayloadWriter writer;
  writer.WriteU64(request.request_id.value());
  writer.WriteU64(request.tab_id.value());
  writer.WriteString(request.url);
  writer.WriteBool(request.is_top_level);
  return MakeNavigationMessage(
      ProcessRole::kBrowser, ProcessRole::kNetwork, kNavigateRequestMessageName, writer.Finish());
}

Message EncodeNavigateResponse(const NavigateResponse& response)
{
  PayloadWriter writer;
  writer.WriteU64(response.request_id.value());
  writer.WriteU8(static_cast<std::uint8_t>(response.status));
  writer.WriteString(response.aegis_reason);
  writer.WriteString(response.error_message);
  writer.WriteString(response.document_body);
  return MakeNavigationMessage(
      ProcessRole::kNetwork, ProcessRole::kBrowser, kNavigateResponseMessageName, writer.Finish());
}

Message EncodeCommitDocument(const CommitDocument& commit)
{
  PayloadWriter writer;
  writer.WriteU64(commit.tab_id.value());
  writer.WriteU64(commit.document_id.value());
  writer.WriteString(commit.url);
  writer.WriteString(commit.document_body);
  return MakeNavigationMessage(
      ProcessRole::kBrowser, ProcessRole::kRenderer, kCommitDocumentMessageName, writer.Finish());
}

Message EncodeCommitErrorPage(const CommitErrorPage& commit)
{
  PayloadWriter writer;
  writer.WriteU64(commit.tab_id.value());
  writer.WriteU64(commit.document_id.value());
  writer.WriteString(commit.url);
  writer.WriteU8(static_cast<std::uint8_t>(commit.reason));
  writer.WriteString(commit.message);
  return MakeNavigationMessage(
      ProcessRole::kBrowser, ProcessRole::kRenderer, kCommitErrorPageMessageName, writer.Finish());
}

Message EncodeRenderReady(const RenderReady& ready)
{
  PayloadWriter writer;
  writer.WriteU64(ready.tab_id.value());
  writer.WriteU64(ready.document_id.value());
  writer.WriteBool(ready.ok);
  writer.WriteBool(ready.is_error_page);
  writer.WriteI32(ready.content_height);
  writer.WriteString(ready.error_message);
  writer.WriteU32(ready.display_commands.size() <= kMaxRenderDisplayCommands
                      ? static_cast<std::uint32_t>(ready.display_commands.size())
                      : 0);
  for (const RenderDisplayCommand& command : ready.display_commands)
  {
    WriteRenderCommand(writer, command);
  }

  return MakeNavigationMessage(
      ProcessRole::kRenderer, ProcessRole::kBrowser, kRenderReadyMessageName, writer.Finish());
}

base::Status DecodeNavigateRequest(const Message& message, NavigateRequest& request)
{
  base::Status status = EnsureNavigationMessage(
      message, ProcessRole::kBrowser, ProcessRole::kNetwork, kNavigateRequestMessageName);
  if (!status.ok())
  {
    return status;
  }

  PayloadReader reader(message.payload());
  std::uint64_t request_id = 0;
  std::uint64_t tab_id = 0;
  if (!reader.ReadU64(request_id) || !reader.ReadU64(tab_id) || !reader.ReadString(request.url) ||
      !reader.ReadBool(request.is_top_level))
  {
    return base::Status::Error("NavigateRequest payload is malformed");
  }

  status = EnsureAtEnd(reader);
  if (!status.ok())
  {
    return status;
  }

  request.request_id = base::RequestId::FromRaw(request_id);
  request.tab_id = base::TabId::FromRaw(tab_id);
  if (!IsValidNavigateRequest(request))
  {
    return base::Status::Error("NavigateRequest payload is invalid");
  }

  return base::Status::Ok();
}

base::Status DecodeNavigateResponse(const Message& message, NavigateResponse& response)
{
  base::Status status = EnsureNavigationMessage(
      message, ProcessRole::kNetwork, ProcessRole::kBrowser, kNavigateResponseMessageName);
  if (!status.ok())
  {
    return status;
  }

  PayloadReader reader(message.payload());
  std::uint64_t request_id = 0;
  std::uint8_t status_value = 0;
  if (!reader.ReadU64(request_id) || !reader.ReadU8(status_value) ||
      !reader.ReadString(response.aegis_reason) || !reader.ReadString(response.error_message) ||
      !reader.ReadString(response.document_body))
  {
    return base::Status::Error("NavigateResponse payload is malformed");
  }

  status = EnsureAtEnd(reader);
  if (!status.ok())
  {
    return status;
  }

  if (status_value > static_cast<std::uint8_t>(NavigateStatus::kFailed))
  {
    return base::Status::Error("NavigateResponse status is invalid");
  }

  response.request_id = base::RequestId::FromRaw(request_id);
  response.status = static_cast<NavigateStatus>(status_value);
  if (!IsValidNavigateResponse(response))
  {
    return base::Status::Error("NavigateResponse payload is invalid");
  }

  return base::Status::Ok();
}

base::Status DecodeCommitDocument(const Message& message, CommitDocument& commit)
{
  base::Status status = EnsureNavigationMessage(
      message, ProcessRole::kBrowser, ProcessRole::kRenderer, kCommitDocumentMessageName);
  if (!status.ok())
  {
    return status;
  }

  PayloadReader reader(message.payload());
  std::uint64_t tab_id = 0;
  std::uint64_t document_id = 0;
  if (!reader.ReadU64(tab_id) || !reader.ReadU64(document_id) || !reader.ReadString(commit.url) ||
      !reader.ReadString(commit.document_body))
  {
    return base::Status::Error("CommitDocument payload is malformed");
  }

  status = EnsureAtEnd(reader);
  if (!status.ok())
  {
    return status;
  }

  commit.tab_id = base::TabId::FromRaw(tab_id);
  commit.document_id = base::DocumentId::FromRaw(document_id);
  if (!IsValidCommitDocument(commit))
  {
    return base::Status::Error("CommitDocument payload is invalid");
  }

  return base::Status::Ok();
}

base::Status DecodeCommitErrorPage(const Message& message, CommitErrorPage& commit)
{
  base::Status status = EnsureNavigationMessage(
      message, ProcessRole::kBrowser, ProcessRole::kRenderer, kCommitErrorPageMessageName);
  if (!status.ok())
  {
    return status;
  }

  PayloadReader reader(message.payload());
  std::uint64_t tab_id = 0;
  std::uint64_t document_id = 0;
  std::uint8_t reason = 0;
  if (!reader.ReadU64(tab_id) || !reader.ReadU64(document_id) || !reader.ReadString(commit.url) ||
      !reader.ReadU8(reason) || !reader.ReadString(commit.message))
  {
    return base::Status::Error("CommitErrorPage payload is malformed");
  }

  status = EnsureAtEnd(reader);
  if (!status.ok())
  {
    return status;
  }

  if (reason > static_cast<std::uint8_t>(ErrorPageReason::kCrashed))
  {
    return base::Status::Error("CommitErrorPage reason is invalid");
  }

  commit.tab_id = base::TabId::FromRaw(tab_id);
  commit.document_id = base::DocumentId::FromRaw(document_id);
  commit.reason = static_cast<ErrorPageReason>(reason);
  if (!IsValidCommitErrorPage(commit))
  {
    return base::Status::Error("CommitErrorPage payload is invalid");
  }

  return base::Status::Ok();
}

base::Status DecodeRenderReady(const Message& message, RenderReady& ready)
{
  base::Status status = EnsureNavigationMessage(
      message, ProcessRole::kRenderer, ProcessRole::kBrowser, kRenderReadyMessageName);
  if (!status.ok())
  {
    return status;
  }

  PayloadReader reader(message.payload());
  std::uint64_t tab_id = 0;
  std::uint64_t document_id = 0;
  std::uint32_t command_count = 0;
  if (!reader.ReadU64(tab_id) || !reader.ReadU64(document_id) || !reader.ReadBool(ready.ok) ||
      !reader.ReadBool(ready.is_error_page) || !reader.ReadI32(ready.content_height) ||
      !reader.ReadString(ready.error_message) || !reader.ReadU32(command_count))
  {
    return base::Status::Error("RenderReady payload is malformed");
  }

  if (command_count > kMaxRenderDisplayCommands)
  {
    return base::Status::Error("RenderReady command count exceeds the limit");
  }

  ready.display_commands.clear();
  ready.display_commands.reserve(command_count);
  for (std::uint32_t index = 0; index < command_count; ++index)
  {
    RenderDisplayCommand command;
    if (!ReadRenderCommand(reader, command))
    {
      return base::Status::Error("RenderReady display command is malformed");
    }
    ready.display_commands.push_back(std::move(command));
  }

  status = EnsureAtEnd(reader);
  if (!status.ok())
  {
    return status;
  }

  ready.tab_id = base::TabId::FromRaw(tab_id);
  ready.document_id = base::DocumentId::FromRaw(document_id);
  if (!IsValidRenderReady(ready))
  {
    return base::Status::Error("RenderReady payload is invalid");
  }

  return base::Status::Ok();
}

} // namespace speed::ipc::navigation
