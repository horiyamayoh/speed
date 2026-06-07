#include "ui/gui/address_bar_model.h"

#include <algorithm>
#include <utility>

namespace speed::ui::gui
{

void AddressBarModel::SetText(std::string text)
{
  text_ = std::move(text);
  caret_ = text_.size();
  ClearSelection();
}

void AddressBarModel::SelectAll()
{
  selection_start_ = 0;
  selection_end_ = text_.size();
  caret_ = text_.size();
}

void AddressBarModel::ClearSelection()
{
  selection_start_ = caret_;
  selection_end_ = caret_;
}

void AddressBarModel::SetCaret(std::size_t caret)
{
  caret_ = std::min(caret, text_.size());
  ClearSelection();
}

void AddressBarModel::InsertChar(char character)
{
  InsertText(std::string_view(&character, 1));
}

void AddressBarModel::InsertText(std::string_view text)
{
  if (text.empty())
  {
    return;
  }

  if (has_selection())
  {
    ReplaceSelection(text);
    return;
  }

  text_.insert(caret_, text);
  caret_ += text.size();
  ClearSelection();
}

void AddressBarModel::Backspace()
{
  if (has_selection())
  {
    ReplaceSelection({});
    return;
  }

  if (caret_ == 0 || text_.empty())
  {
    return;
  }

  text_.erase(caret_ - 1, 1);
  --caret_;
  ClearSelection();
}

const std::string& AddressBarModel::text() const
{
  return text_;
}

std::size_t AddressBarModel::caret() const
{
  return caret_;
}

bool AddressBarModel::has_selection() const
{
  return selection_start_ != selection_end_;
}

bool AddressBarModel::all_selected() const
{
  return has_selection() && selection_start_ == 0 && selection_end_ == text_.size();
}

std::size_t AddressBarModel::selection_start() const
{
  return std::min(selection_start_, selection_end_);
}

std::size_t AddressBarModel::selection_end() const
{
  return std::max(selection_start_, selection_end_);
}

void AddressBarModel::ReplaceSelection(std::string_view text)
{
  const std::size_t start = selection_start();
  const std::size_t end = selection_end();
  text_.replace(start, end - start, text);
  caret_ = start + text.size();
  ClearSelection();
  ClampState();
}

void AddressBarModel::ClampState()
{
  caret_ = std::min(caret_, text_.size());
  selection_start_ = std::min(selection_start_, text_.size());
  selection_end_ = std::min(selection_end_, text_.size());
}

} // namespace speed::ui::gui
