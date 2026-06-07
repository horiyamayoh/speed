#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace
{

struct ForbiddenIncludeRule final
{
  std::filesystem::path relative_root;
  std::vector<std::string_view> forbidden_patterns;
};

[[nodiscard]] bool FileContainsAny(std::string_view contents,
                                   const std::vector<std::string_view>& patterns)
{
  for (std::string_view pattern : patterns)
  {
    if (contents.find(pattern) != std::string_view::npos)
    {
      return true;
    }
  }

  return false;
}

[[nodiscard]] std::string ReadFile(const std::filesystem::path& path)
{
  std::ifstream input(path);
  return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void AssertRule(const std::filesystem::path& source_root, const ForbiddenIncludeRule& rule)
{
  const std::filesystem::path root = source_root / rule.relative_root;
  if (!std::filesystem::exists(root))
  {
    return;
  }

  for (const std::filesystem::directory_entry& entry :
       std::filesystem::recursive_directory_iterator(root))
  {
    if (!entry.is_regular_file())
    {
      continue;
    }

    const std::filesystem::path extension = entry.path().extension();
    if (extension != ".h" && extension != ".cpp")
    {
      continue;
    }

    const std::string contents = ReadFile(entry.path());
    assert(!FileContainsAny(contents, rule.forbidden_patterns));
  }
}

} // namespace

int main()
{
  const std::filesystem::path source_root = std::filesystem::path(SPEED_SOURCE_DIR) / "src";
  const std::vector<ForbiddenIncludeRule> rules = {
      {
          .relative_root = "renderer",
          .forbidden_patterns = {"#include \"network/", "#include \"storage/"},
      },
      {
          .relative_root = "network",
          .forbidden_patterns = {"#include \"browser/",
                                 "#include \"renderer/",
                                 "#include \"ui/",
                                 "#include \"storage/"},
      },
      {
          .relative_root = "engine",
          .forbidden_patterns = {"#include \"browser/",
                                 "#include \"network/",
                                 "#include \"storage/",
                                 "#include \"ui/"},
      },
      {
          .relative_root = "aegis",
          .forbidden_patterns = {"#include \"network/", "#include \"ui/"},
      },
      {
          .relative_root = "ipc",
          .forbidden_patterns = {"#include \"browser/",
                                 "#include \"network/",
                                 "#include \"renderer/",
                                 "#include \"engine/",
                                 "#include \"ui/",
                                 "#include \"storage/"},
      },
  };

  for (const ForbiddenIncludeRule& rule : rules)
  {
    AssertRule(source_root, rule);
  }

  return 0;
}
