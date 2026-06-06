#include "ipc/runtime/message.h"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace speed::ipc
{

Message::Message(MessageHeader header, std::string payload)
    : header_(std::move(header)),
      payload_(std::move(payload))
{}

const MessageHeader& Message::header() const
{
  return header_;
}

const std::string& Message::payload() const
{
  return payload_;
}

bool IsKnownSchema(std::string_view schema_name)
{
  constexpr std::array<std::string_view, 1> kKnownSchemas = {
      "speed.navigation.v0",
  };

  return std::ranges::any_of(kKnownSchemas,
                             [schema_name](const std::string_view known_schema)
                             { return known_schema == schema_name; });
}

} // namespace speed::ipc
