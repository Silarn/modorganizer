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

// forTuple and forArgs based on/from https://www.youtube.com/watch?v=Za92Tz_g0zQ
template <typename TF, typename... Ts>
void forArgs(TF&& mFn, Ts&&... mArgs) {
    return (void)std::initializer_list<int>{(mFn(std::forward<Ts>(mArgs)), 0)...};
}

// `forTuple` is a function that takes a callable object and
// and `std::tuple` as its parameters.
//
// It then calls the passed function individually passing every
// element of the tuple as its argument.
template <typename TFn, typename TTpl>
void forTuple(TFn&& mFn, TTpl&& mTpl) {
    // We basically expand the tuple into a function call to
    // a variadic polymorphic lambda with `apply`, which in
    // turn passes the expanded tuple elements to `forArgs`,
    // one by one... which in turn calls `mFn` with every
    // single tuple element individually.

    std::apply([&mFn](auto&&... xs) { forArgs(mFn, std::forward<decltype(xs)>(xs)...); }, std::forward<TTpl>(mTpl));
}
} // namespace common
