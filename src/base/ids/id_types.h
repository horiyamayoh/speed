#pragma once

#include <compare>
#include <cstdint>

namespace speed::base
{

template <typename Tag> class Id final
{
public:
  constexpr Id() = default;

  [[nodiscard]] static constexpr Id FromRaw(std::uint64_t value)
  {
    return Id(value);
  }

  [[nodiscard]] constexpr std::uint64_t value() const
  {
    return value_;
  }

  constexpr explicit operator bool() const
  {
    return value_ != 0;
  }

  friend bool operator==(const Id&, const Id&) = default;
  friend auto operator<=>(const Id&, const Id&) = default;

private:
  explicit constexpr Id(std::uint64_t value)
      : value_(value)
  {}

  std::uint64_t value_{0};
};

struct TabIdTag;
struct ProcessIdTag;
struct RequestIdTag;
struct DocumentIdTag;

using TabId = Id<TabIdTag>;
using ProcessId = Id<ProcessIdTag>;
using RequestId = Id<RequestIdTag>;
using DocumentId = Id<DocumentIdTag>;

} // namespace speed::base
