// Useful general utilities
#pragma once
#include <filesystem>
#include <string>
namespace fs = std::experimental::filesystem;

namespace common {
// Get the directory the Executable is located.
fs::path get_exe_dir();

// Whether the directory at path `dir` is writeable
// Returns false is the directory is not writeable, doesnt exist, or is not really a directory.
bool is_writeable(fs::path dr);

// Open URL `url` in the systems default web browser
void open_url(std::string url);
void open_url(std::wstring url);
} // namespace common
