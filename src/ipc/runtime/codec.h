#pragma once

#include "base/result/status.h"
#include "ipc/runtime/message.h"

#include <string>
#include <string_view>

namespace speed::ipc
{

[[nodiscard]] std::string EncodeMessage(const Message& message);
[[nodiscard]] base::Status DecodeMessage(std::string_view bytes, Message& message);

} // namespace speed::ipc
