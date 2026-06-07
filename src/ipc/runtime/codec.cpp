#include "ipc/runtime/codec.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace speed::ipc
{

namespace
{

constexpr std::string_view kMagic = "SPIP";
constexpr std::uint8_t kEncodingVersion = 1;

[[nodiscard]] bool IsKnownRole(std::uint8_t value)
{
  return value <= static_cast<std::uint8_t>(ProcessRole::kUtility);
}

void AppendU8(std::string& output, std::uint8_t value)
{
  output.push_back(static_cast<char>(value));
}

void AppendU32(std::string& output, std::uint32_t value)
{
  for (int shift = 24; shift >= 0; shift -= 8)
  {
    output.push_back(static_cast<char>((value >> shift) & 0xffU));
  }
}

void AppendBytes(std::string& output, std::string_view bytes)
{
  output.append(bytes.data(), bytes.size());
}

[[nodiscard]] bool ReadU8(std::string_view bytes, std::size_t& offset, std::uint8_t& value)
{
  if (offset >= bytes.size())
  {
    return false;
  }

  value = static_cast<std::uint8_t>(bytes[offset]);
  ++offset;
  return true;
}

[[nodiscard]] bool ReadU32(std::string_view bytes, std::size_t& offset, std::uint32_t& value)
{
  if (bytes.size() - offset < sizeof(std::uint32_t))
  {
    return false;
  }

  value = 0;
  for (int index = 0; index < 4; ++index)
  {
    value <<= 8U;
    value |= static_cast<std::uint8_t>(bytes[offset + static_cast<std::size_t>(index)]);
  }
  offset += sizeof(std::uint32_t);
  return true;
}

[[nodiscard]] bool
ReadString(std::string_view bytes, std::size_t& offset, std::uint32_t size, std::string& value)
{
  if (bytes.size() - offset < size)
  {
    return false;
  }

  value = std::string(bytes.substr(offset, size));
  offset += size;
  return true;
}

[[nodiscard]] bool FitsU32(std::size_t value)
{
  return value <= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
}

} // namespace

std::string EncodeMessage(const Message& message)
{
  std::string output;
  const MessageHeader& header = message.header();
  output.reserve(kMagic.size() + 32 + header.schema_name.size() + header.message_name.size() +
                 message.payload().size());

  AppendBytes(output, kMagic);
  AppendU8(output, kEncodingVersion);
  AppendU8(output, static_cast<std::uint8_t>(header.sender));
  AppendU8(output, static_cast<std::uint8_t>(header.receiver));
  AppendU32(output, header.schema_version);
  AppendU32(output,
            FitsU32(header.schema_name.size())
                ? static_cast<std::uint32_t>(header.schema_name.size())
                : 0);
  AppendU32(output,
            FitsU32(header.message_name.size())
                ? static_cast<std::uint32_t>(header.message_name.size())
                : 0);
  AppendU32(output,
            FitsU32(message.payload().size()) ? static_cast<std::uint32_t>(message.payload().size())
                                              : 0);
  AppendBytes(output, header.schema_name);
  AppendBytes(output, header.message_name);
  AppendBytes(output, message.payload());
  return output;
}

base::Status DecodeMessage(std::string_view bytes, Message& message)
{
  std::size_t offset = 0;
  if (bytes.size() < kMagic.size() || bytes.substr(0, kMagic.size()) != kMagic)
  {
    return base::Status::Error("IPC frame has invalid magic");
  }
  offset += kMagic.size();

  std::uint8_t encoding_version = 0;
  std::uint8_t sender = 0;
  std::uint8_t receiver = 0;
  std::uint32_t schema_version = 0;
  std::uint32_t schema_name_size = 0;
  std::uint32_t message_name_size = 0;
  std::uint32_t payload_size = 0;
  if (!ReadU8(bytes, offset, encoding_version) || encoding_version != kEncodingVersion)
  {
    return base::Status::Error("IPC frame has unsupported encoding version");
  }

  if (!ReadU8(bytes, offset, sender) || !ReadU8(bytes, offset, receiver) ||
      !ReadU32(bytes, offset, schema_version) || !ReadU32(bytes, offset, schema_name_size) ||
      !ReadU32(bytes, offset, message_name_size) || !ReadU32(bytes, offset, payload_size))
  {
    return base::Status::Error("IPC frame header is truncated");
  }

  if (!IsKnownRole(sender) || !IsKnownRole(receiver))
  {
    return base::Status::Error("IPC frame has unknown process role");
  }

  std::string schema_name;
  std::string message_name;
  std::string payload;
  if (!ReadString(bytes, offset, schema_name_size, schema_name) ||
      !ReadString(bytes, offset, message_name_size, message_name) ||
      !ReadString(bytes, offset, payload_size, payload))
  {
    return base::Status::Error("IPC frame body is truncated");
  }

  if (offset != bytes.size())
  {
    return base::Status::Error("IPC frame has trailing bytes");
  }

  if (schema_name.empty() || !IsKnownSchema(schema_name))
  {
    return base::Status::Error("IPC frame references an unknown schema");
  }

  if (schema_version == 0)
  {
    return base::Status::Error("IPC frame schema version is invalid");
  }

  if (message_name.empty())
  {
    return base::Status::Error("IPC frame message name is empty");
  }

  message = Message(
      {
          .sender = static_cast<ProcessRole>(sender),
          .receiver = static_cast<ProcessRole>(receiver),
          .schema_name = std::move(schema_name),
          .schema_version = schema_version,
          .message_name = std::move(message_name),
      },
      std::move(payload));
  return base::Status::Ok();
}

} // namespace speed::ipc
