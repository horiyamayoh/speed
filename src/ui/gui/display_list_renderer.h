#pragma once

#include "engine/paint/display_list.h"
#include "platform/window/canvas.h"

namespace speed::ui::gui
{

class DisplayListRenderer final
{
public:
  void Render(const engine::paint::DisplayList& display_list,
              platform::window::Canvas& canvas,
              platform::window::Rect viewport,
              int scroll_y) const;
};

} // namespace speed::ui::gui
