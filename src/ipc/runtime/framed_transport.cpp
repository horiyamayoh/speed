#include "ipc/runtime/framed_transport.h"

#include "ipc/runtime/codec.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace speed::ipc
{

namespace
{

[[nodiscard]] base::Status ErrnoStatus(std::string_view prefix)
{
  return base::Status::Error(std::string(prefix) + ": " + std::strerror(errno));
}

void EncodeFrameSize(std::uint32_t size, char output[4])
{
  output[0] = static_cast<char>((size >> 24U) & 0xffU);
  output[1] = static_cast<char>((size >> 16U) & 0xffU);
  output[2] = static_cast<char>((size >> 8U) & 0xffU);
  output[3] = static_cast<char>(size & 0xffU);
}

[[nodiscard]] std::uint32_t DecodeFrameSize(const char input[4])
{
  std::uint32_t size = 0;
  for (int index = 0; index < 4; ++index)
  {
    size <<= 8U;
    size |= static_cast<std::uint8_t>(input[index]);
  }
  return size;
}

[[nodiscard]] base::Status WriteAll(int file_descriptor, std::string_view bytes)
{
  while (!bytes.empty())
  {
    const ssize_t written = ::send(file_descriptor, bytes.data(), bytes.size(), MSG_NOSIGNAL);
    if (written < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      return ErrnoStatus("failed to write IPC frame");
    }

    if (written == 0)
    {
      return base::Status::Error("IPC transport closed while writing frame");
    }

    bytes.remove_prefix(static_cast<std::size_t>(written));
  }

  return base::Status::Ok();
}

[[nodiscard]] base::Status ReadAll(int file_descriptor, char* output, std::size_t size)
{
  std::size_t read_so_far = 0;
  while (read_so_far < size)
  {
    const ssize_t read_count = ::read(file_descriptor, output + read_so_far, size - read_so_far);
    if (read_count < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      return ErrnoStatus("failed to read IPC frame");
    }

    if (read_count == 0)
    {
      return base::Status::Error("IPC transport closed while reading frame");
    }

    read_so_far += static_cast<std::size_t>(read_count);
  }

  return base::Status::Ok();
}

} // namespace

FileDescriptorTransport::FileDescriptorTransport(int file_descriptor)
    : file_descriptor_(file_descriptor)
{}

FileDescriptorTransport::~FileDescriptorTransport()
{
  if (file_descriptor_ >= 0)
  {
    (void)::close(file_descriptor_);
  }
}

FileDescriptorTransport::FileDescriptorTransport(FileDescriptorTransport&& other) noexcept
    : file_descriptor_(std::exchange(other.file_descriptor_, -1))
{}

FileDescriptorTransport&
FileDescriptorTransport::operator=(FileDescriptorTransport&& other) noexcept
{
  if (this != &other)
  {
    if (file_descriptor_ >= 0)
    {
      (void)::close(file_descriptor_);
    }
    file_descriptor_ = std::exchange(other.file_descriptor_, -1);
  }
  return *this;
}

bool FileDescriptorTransport::valid() const
{
  return file_descriptor_ >= 0;
}

int FileDescriptorTransport::file_descriptor() const
{
  return file_descriptor_;
}

int FileDescriptorTransport::Release()
{
  return std::exchange(file_descriptor_, -1);
}

base::Status FileDescriptorTransport::SendMessage(const Message& message) const
{
  if (!valid())
  {
    return base::Status::Error("IPC transport file descriptor is invalid");
  }

  const std::string encoded_message = EncodeMessage(message);
  if (encoded_message.size() > kMaxIpcFrameBytes)
  {
    return base::Status::Error("IPC frame exceeds maximum size");
  }

  char frame_size[4]{};
  EncodeFrameSize(static_cast<std::uint32_t>(encoded_message.size()), frame_size);
  base::Status status =
      WriteAll(file_descriptor_, std::string_view(frame_size, sizeof(frame_size)));
  if (!status.ok())
  {
    return status;
  }

  return WriteAll(file_descriptor_, encoded_message);
}

base::Status FileDescriptorTransport::ReceiveMessage(Message& message) const
{
  if (!valid())
  {
    return base::Status::Error("IPC transport file descriptor is invalid");
  }

  char frame_size_bytes[4]{};
  base::Status status = ReadAll(file_descriptor_, frame_size_bytes, sizeof(frame_size_bytes));
  if (!status.ok())
  {
    return status;
  }

  const std::uint32_t frame_size = DecodeFrameSize(frame_size_bytes);
  if (frame_size == 0 || frame_size > kMaxIpcFrameBytes)
  {
    return base::Status::Error("IPC frame size is invalid");
  }

  std::string frame(frame_size, '\0');
  status = ReadAll(file_descriptor_, frame.data(), frame.size());
  if (!status.ok())
  {
    return status;
  }

  return DecodeMessage(frame, message);
}

base::Status CreateLocalTransportPair(LocalTransportPair& pair)
{
  int file_descriptors[2]{-1, -1};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, file_descriptors) != 0)
  {
    return ErrnoStatus("failed to create IPC socketpair");
  }

  pair.first = FileDescriptorTransport(file_descriptors[0]);
  pair.second = FileDescriptorTransport(file_descriptors[1]);
  return base::Status::Ok();
}

} // namespace speed::ipc
