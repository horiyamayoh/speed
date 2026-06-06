#pragma once

#include <string>
#include <utility>

namespace speed::base
{

class Status final
{
public:
  static Status Ok()
  {
    return {true, {}};
  }

  static Status Error(std::string message)
  {
    return {false, std::move(message)};
  }

  [[nodiscard]] bool ok() const
  {
    return ok_;
  }

  [[nodiscard]] const std::string& message() const
  {
    return message_;
  }

private:
  Status(bool ok, std::string message)
      : ok_(ok),
        message_(std::move(message))
  {}

  bool ok_{true};
  std::string message_;
};

} // namespace speed::base
