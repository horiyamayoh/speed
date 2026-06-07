#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace speed::ui::gui
{

class AddressBarModel final
{
public:
  void SetText(std::string text);
  void SelectAll();
  void ClearSelection();
  void SetCaret(std::size_t caret);
  void InsertChar(char character);
  void InsertText(std::string_view text);
  void Backspace();

  [[nodiscard]] const std::string& text() const;
  [[nodiscard]] std::size_t caret() const;
  [[nodiscard]] bool has_selection() const;
  [[nodiscard]] bool all_selected() const;
  [[nodiscard]] std::size_t selection_start() const;
  [[nodiscard]] std::size_t selection_end() const;

private:
  void ReplaceSelection(std::string_view text);
  void ClampState();

  std::string text_;
  std::size_t caret_{0};
  std::size_t selection_start_{0};
  std::size_t selection_end_{0};
};

} // namespace speed::ui::gui
