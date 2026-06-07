#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace speed::engine::dom
{

enum class NodeType : std::uint8_t
{
  kDocument,
  kElement,
  kText,
};

struct Attribute final
{
  std::string name;
  std::string value;
};

struct Node final
{
  NodeType type{NodeType::kDocument};
  std::string name;
  std::string text;
  std::vector<Attribute> attributes;
  std::vector<Node> children;
};

} // namespace speed::engine::dom
