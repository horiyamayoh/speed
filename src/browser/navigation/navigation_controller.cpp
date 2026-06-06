#include "browser/navigation/navigation_controller.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace speed::browser
{

namespace
{

[[nodiscard]] bool IsAsciiWhitespace(char character)
{
  return character == ' ' || character == '\t' || character == '\n' || character == '\r' ||
         character == '\f' || character == '\v';
}

[[nodiscard]] char ToAsciiLower(char character)
{
  if (character >= 'A' && character <= 'Z')
  {
    return static_cast<char>(character - 'A' + 'a');
  }

  return character;
}

[[nodiscard]] std::string ToAsciiLower(std::string_view input)
{
  std::string result(input);
  for (char& character : result)
  {
    character = ToAsciiLower(character);
  }
  return result;
}

[[nodiscard]] std::string TrimAsciiWhitespace(std::string_view input)
{
  std::size_t begin = 0;
  while (begin < input.size() && IsAsciiWhitespace(input[begin]))
  {
    ++begin;
  }

  std::size_t end = input.size();
  while (end > begin && IsAsciiWhitespace(input[end - 1]))
  {
    --end;
  }

  return std::string(input.substr(begin, end - begin));
}

[[nodiscard]] bool ContainsAsciiWhitespace(std::string_view input)
{
  for (const char character : input)
  {
    if (IsAsciiWhitespace(character))
    {
      return true;
    }
  }

  return false;
}

[[nodiscard]] std::string NormalizeHttpAuthority(std::string url)
{
  const std::string_view separator = "://";
  const std::size_t scheme_end = url.find(separator);
  if (scheme_end == std::string::npos)
  {
    return url;
  }

  const std::size_t authority_begin = scheme_end + separator.size();
  std::size_t authority_end = url.find_first_of("/?#", authority_begin);
  if (authority_end == std::string::npos)
  {
    authority_end = url.size();
  }

  for (std::size_t index = authority_begin; index < authority_end; ++index)
  {
    url[index] = ToAsciiLower(url[index]);
  }

  return url;
}

[[nodiscard]] std::string Scheme(std::string_view input)
{
  const std::size_t scheme_end = input.find(':');
  if (scheme_end == std::string_view::npos || scheme_end == 0)
  {
    return {};
  }

  return ToAsciiLower(input.substr(0, scheme_end));
}

[[nodiscard]] bool HasHierarchicalScheme(std::string_view input)
{
  const std::size_t scheme_end = input.find(':');
  if (scheme_end == std::string_view::npos)
  {
    return false;
  }

  return input.substr(scheme_end).starts_with("://");
}

[[nodiscard]] bool IsSupportedNavigationUrl(std::string_view url)
{
  const std::string scheme = Scheme(url);
  if (scheme == "about")
  {
    return url == "about:blank";
  }

  if (scheme != "http" && scheme != "https")
  {
    return false;
  }

  const std::string_view separator = "://";
  const std::size_t scheme_end = url.find(':');
  if (!url.substr(scheme_end).starts_with(separator))
  {
    return false;
  }

  const std::size_t authority_begin = scheme_end + separator.size();
  if (authority_begin >= url.size())
  {
    return false;
  }

  return url[authority_begin] != '/' && url[authority_begin] != '?' && url[authority_begin] != '#';
}

} // namespace

std::string NavigationController::NormalizeForNavigation(std::string_view input) const
{
  const std::string trimmed = TrimAsciiWhitespace(input);
  if (trimmed.empty())
  {
    return {};
  }

  const std::string scheme = Scheme(trimmed);
  if (scheme == "about")
  {
    const std::size_t scheme_end = trimmed.find(':');
    return "about:" + ToAsciiLower(std::string_view(trimmed).substr(scheme_end + 1));
  }

  if (HasHierarchicalScheme(trimmed))
  {
    const std::size_t scheme_end = trimmed.find(':');
    std::string normalized = scheme + std::string(std::string_view(trimmed).substr(scheme_end));
    if (scheme == "http" || scheme == "https")
    {
      return NormalizeHttpAuthority(std::move(normalized));
    }

    return normalized;
  }

  return NormalizeHttpAuthority("https://" + trimmed);
}

std::optional<NavigationRequest>
NavigationController::CreateNavigationRequest(base::TabId tab_id, std::string_view input)
{
  if (!tab_id)
  {
    return std::nullopt;
  }

  const std::string normalized_url = NormalizeForNavigation(input);
  if (normalized_url.empty() || ContainsAsciiWhitespace(normalized_url) ||
      !IsSupportedNavigationUrl(normalized_url))
  {
    return std::nullopt;
  }

  return NavigationRequest{
      .request_id = base::RequestId::FromRaw(next_request_id_++),
      .tab_id = tab_id,
      .url = normalized_url,
  };
}

} // namespace speed::browser
