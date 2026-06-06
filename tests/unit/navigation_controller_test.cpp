#include "browser/navigation/navigation_controller.h"

#include <cassert>
#include <optional>

int main()
{
  speed::browser::NavigationController controller;

  assert(controller.NormalizeForNavigation(" example.test/path ") == "https://example.test/path");
  assert(controller.NormalizeForNavigation("HTTP://Example.TEST/Path") ==
         "http://example.test/Path");
  assert(controller.NormalizeForNavigation("about:BLANK") == "about:blank");
  assert(controller.NormalizeForNavigation("localhost:8080/status") ==
         "https://localhost:8080/status");

  const speed::base::TabId tab_id = speed::base::TabId::FromRaw(7);
  const std::optional<speed::browser::NavigationRequest> first_request =
      controller.CreateNavigationRequest(tab_id, "speed.test");
  assert(first_request.has_value());
  assert(first_request->request_id.value() == 1);
  assert(first_request->tab_id == tab_id);
  assert(first_request->url == "https://speed.test");

  const std::optional<speed::browser::NavigationRequest> blank_request =
      controller.CreateNavigationRequest(tab_id, "about:blank");
  assert(blank_request.has_value());
  assert(blank_request->request_id.value() == 2);
  assert(blank_request->url == "about:blank");

  assert(!controller.CreateNavigationRequest({}, "speed.test").has_value());
  assert(!controller.CreateNavigationRequest(tab_id, "   ").has_value());
  assert(!controller.CreateNavigationRequest(tab_id, "file:///tmp/speed").has_value());
  assert(!controller.CreateNavigationRequest(tab_id, "https://example.test/a b").has_value());

  const std::optional<speed::browser::NavigationRequest> next_request =
      controller.CreateNavigationRequest(tab_id, "https://Example.TEST/");
  assert(next_request.has_value());
  assert(next_request->request_id.value() == 3);
  assert(next_request->url == "https://example.test/");

  return 0;
}
