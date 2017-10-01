// Definition point for functions defined in various headers.
#include "common/sane_windows.h"
#include "common/util.h"

#include <assert.h>
#include <string>
#include <vector>
namespace common {

fs::path get_exe_dir() {
    // Since this won't change, cache the result.
    static bool runOnce = false;
    static fs::path ret;
    if (runOnce) {
        return ret;
    }
    runOnce = true;
    std::wstring path;
    DWORD bufLen = 0;
    // Allocate more memory until this horrible API succeeds.
    // the only way to tell if it worked is if the shitty function returns a size less than our buffers size.
    // If it fails, it silently truncates the result and returns the buffer size.
    // So we have to allocate more memory until then.
    do {
        path.resize(path.size() + MAX_PATH);
        bufLen = GetModuleFileNameW(NULL, path.data(), path.size());
        // Assert we're getting the expected errors.
        assert((GetLastError() == ERROR_INSUFFICIENT_BUFFER) || GetLastError() == 0);
        // Reset error state.
        assert((SetLastError(0), 1));
    } while (bufLen >= path.size());
    path.resize(bufLen);
    ret = path;
    ret = ret.parent_path();
    return ret;
}

bool is_writeable(fs::path dir) {
    if (!fs::exists(dir) || fs::is_directory(dir)) {
        return false;
    }
    auto perms = fs::status(dir).permissions();
    return (perms & fs::perms::owner_write) != fs::perms::none;
}

} // namespace common
