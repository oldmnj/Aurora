module;

#include <fmt/format.h>

module launcher.base;
namespace launcher {
String Version::ToString() const { return fmt::format("{}.{}.{}", major, minor, patch); }
}  // namespace launcher
