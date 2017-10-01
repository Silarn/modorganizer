// Useful general utilities
#pragma once
#include <filesystem>
namespace fs = std::experimental::filesystem;

namespace common {
// Get the directory the Executable is located.
fs::path get_exe_dir();
} // namespace common
