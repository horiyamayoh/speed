#pragma once

#include "base/ids/id_types.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace speed::network
{

enum class FetchStatus : std::uint8_t
{
  kSucceeded,
  kFailed,
};

struct FetchRequest final
{
  base::RequestId request_id;
  std::string url;
};

struct FetchResult final
{
  FetchStatus status{FetchStatus::kFailed};
  std::string body;
  std::string error_message;

  [[nodiscard]] bool ok() const
  {
    return status == FetchStatus::kSucceeded;
  }

  [[nodiscard]] static FetchResult Success(std::string body)
  {
    return {
        .status = FetchStatus::kSucceeded,
        .body = std::move(body),
        .error_message = {},
    };
  }

  [[nodiscard]] static FetchResult Failure(std::string error_message)
  {
    return {
        .status = FetchStatus::kFailed,
        .body = {},
        .error_message = std::move(error_message),
    };
  }
};

class FetchAdapter
{
public:
  virtual ~FetchAdapter() = default;

  [[nodiscard]] virtual FetchResult Fetch(const FetchRequest& request) const = 0;
};

[[nodiscard]] std::unique_ptr<FetchAdapter> CreateDefaultFetchAdapter();

} // namespace speed::network
