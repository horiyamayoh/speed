#pragma once

#include "aegis/classifier/request_classifier.h"
#include "base/ids/id_types.h"
#include "ipc/runtime/navigation_messages.h"

#include <string>

namespace speed::network
{

struct NetworkRequest final
{
  base::RequestId request_id;
  std::string url;
};

struct NetworkResult final
{
  bool would_dispatch{false};
  aegis::Classification aegis_decision;
};

class NetworkService final
{
public:
  explicit NetworkService(aegis::RequestClassifier classifier = {});

  [[nodiscard]] NetworkResult PrepareRequest(const NetworkRequest& request) const;
  [[nodiscard]] ipc::navigation::NavigateResponse
  FetchNavigation(const ipc::navigation::NavigateRequest& request) const;

private:
  aegis::RequestClassifier classifier_;
};

} // namespace speed::network
