#include "ui/gui/address_bar_model.h"

#include <cassert>

int main()
{
  speed::ui::gui::AddressBarModel model;
  model.SetText("https://example.test");
  assert(model.caret() == model.text().size());
  assert(!model.has_selection());

  model.SelectAll();
  assert(model.all_selected());
  model.InsertText("about:blank");
  assert(model.text() == "about:blank");
  assert(model.caret() == model.text().size());
  assert(!model.has_selection());

  model.SelectAll();
  model.Backspace();
  assert(model.text().empty());
  assert(model.caret() == 0);

  model.SetText("abcd");
  model.SetCaret(2);
  model.InsertChar('X');
  assert(model.text() == "abXcd");
  assert(model.caret() == 3);
  model.Backspace();
  assert(model.text() == "abcd");
  assert(model.caret() == 2);

  return 0;
}
