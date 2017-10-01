// Useful general utilities
#pragma once
#include <filesystem>
namespace fs = std::experimental::filesystem;

namespace common {
// Get the directory the Executable is located.
fs::path get_exe_dir();

// Whether the directory at path `dir` is writeable
// Returns false is the directory is not writeable, doesnt exist, or is not really a directory.
bool is_writeable(fs::path dr);
} // namespace common
