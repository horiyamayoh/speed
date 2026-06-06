#include "renderer/document/renderer_process.h"

#include "base/logging/logging.h"
#include "engine/html/html_parser.h"

namespace speed::renderer
{

int RunRendererProcess()
{
  const engine::html::HtmlParser parser;
  const engine::dom::Node document = parser.ParseFragment("<html><body></body></html>");
  (void)document;

  base::Log(base::LogLevel::kInfo, "renderer", "renderer process stub ready");
  return 0;
}

} // namespace speed::renderer
