#include "ipc/runtime/message.h"

#include <cassert>
#include <filesystem>

int main()
{
  assert(speed::ipc::IsKnownSchema("speed.navigation.v0"));
  assert(!speed::ipc::IsKnownSchema("speed.unknown.v0"));

  const std::filesystem::path schema_path =
      std::filesystem::path(SPEED_SOURCE_DIR) / "src/ipc/schemas/navigation.speedipc";
  assert(std::filesystem::exists(schema_path));

  return 0;
}
