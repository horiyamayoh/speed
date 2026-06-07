#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace speed::ipc
{

enum class ProcessRole : std::uint8_t
{
  kBrowser,
  kRenderer,
  kNetwork,
  kUtility,
};

struct MessageHeader final
{
  ProcessRole sender{ProcessRole::kBrowser};
  ProcessRole receiver{ProcessRole::kBrowser};
  std::string schema_name;
  std::uint32_t schema_version{0};
  std::string message_name;
};

class Message final
{
public:
  Message(MessageHeader header, std::string payload);

  [[nodiscard]] const MessageHeader& header() const;
  [[nodiscard]] const std::string& payload() const;

private:
  MessageHeader header_;
  std::string payload_;
};

[[nodiscard]] bool IsKnownSchema(std::string_view schema_name);

} // namespace speed::ipc
