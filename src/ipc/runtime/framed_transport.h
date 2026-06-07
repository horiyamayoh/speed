#pragma once

#include "base/result/status.h"
#include "ipc/runtime/message.h"

#include <cstdint>

namespace speed::ipc
{

class FileDescriptorTransport final
{
public:
  FileDescriptorTransport() = default;
  explicit FileDescriptorTransport(int file_descriptor);
  ~FileDescriptorTransport();

  FileDescriptorTransport(const FileDescriptorTransport&) = delete;
  FileDescriptorTransport& operator=(const FileDescriptorTransport&) = delete;
  FileDescriptorTransport(FileDescriptorTransport&& other) noexcept;
  FileDescriptorTransport& operator=(FileDescriptorTransport&& other) noexcept;

  [[nodiscard]] bool valid() const;
  [[nodiscard]] int file_descriptor() const;
  [[nodiscard]] int Release();

  [[nodiscard]] base::Status SendMessage(const Message& message) const;
  [[nodiscard]] base::Status ReceiveMessage(Message& message) const;

private:
  int file_descriptor_{-1};
};

struct LocalTransportPair final
{
  FileDescriptorTransport first;
  FileDescriptorTransport second;
};

inline constexpr std::uint32_t kMaxIpcFrameBytes = 8U * 1024U * 1024U;

[[nodiscard]] base::Status CreateLocalTransportPair(LocalTransportPair& pair);

} // namespace speed::ipc
