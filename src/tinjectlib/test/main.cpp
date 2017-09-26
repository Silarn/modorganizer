#define GTEST_LANG_CXX11 1
#include "tinjectlib/injectlib.h"
#include "usvfs_shared/winapi.h"
#include "usvfs_shared/windows_error.h"
#include <filesystem>
#include <gtest/gtest.h>
namespace fs = std::experimental::filesystem;

using namespace usvfs::shared;
using namespace InjectLib;

static const std::wstring INJECT_LIB = L"tinjectlibTestDll.dll";
static const std::wstring INJECT_EXE = L"tinjectlibTestExe.exe";

bool spawn(HANDLE& processHandle, HANDLE& threadHandle) {
    STARTUPINFO si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL success = ::CreateProcessW(INJECT_EXE.data(), nullptr, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr,
                                    nullptr, &si, &pi);

    if (!success) {
        throw windows_error("failed to start process");
    }

    processHandle = pi.hProcess;
    threadHandle = pi.hThread;

    return true;
}

TEST(InjectingTest, InjectionNoInit) {
    // Verify lib can inject without a init function

    HANDLE process, thread;
    spawn(process, thread);
    EXPECT_NO_THROW(InjectLib::InjectDLL(process, thread, INJECT_LIB.data()));
    ResumeThread(thread);

    DWORD res = WaitForSingleObject(process, INFINITE);
    DWORD exitCode = NO_ERROR;
    res = GetExitCodeProcess(process, &exitCode);
    EXPECT_EQ(NOERROR, exitCode);

    CloseHandle(process);
    CloseHandle(thread);
}

TEST(InjectingTest, InjectionSimpleInit) {
    // Verify lib can inject with a init function with null parameters
    HANDLE process;
    HANDLE thread;
    spawn(process, thread);
    EXPECT_NO_THROW(InjectLib::InjectDLL(process, thread, INJECT_LIB.data(), "InitNoParam"));
    EXPECT_EQ(ResumeThread(thread), 1);

    DWORD res = WaitForSingleObject(process, INFINITE);
    EXPECT_EQ(res, WAIT_OBJECT_0);
    DWORD exitCode = NO_ERROR;
    res = GetExitCodeProcess(process, &exitCode);
    EXPECT_TRUE(res);
    EXPECT_NE(res, STILL_ACTIVE);
    EXPECT_EQ(10001, exitCode); // used init function exits process with this exit code

    CloseHandle(process);
    CloseHandle(thread);
}

TEST(InjectingTest, InjectionComplexInit) {
    // Verify lib can inject with a init function with null parameters

    static const WCHAR param[] = L"magic_parameter";
    HANDLE process, thread;
    spawn(process, thread);
    EXPECT_NO_THROW(InjectLib::InjectDLL(process, thread, INJECT_LIB.data(), "InitComplexParam",
                                         reinterpret_cast<LPCVOID>(param), wcslen(param) * sizeof(WCHAR)));

    ResumeThread(thread);

    DWORD res = WaitForSingleObject(process, INFINITE);
    DWORD exitCode = NO_ERROR;
    res = GetExitCodeProcess(process, &exitCode);
    EXPECT_EQ(10002, exitCode); // used init function exits process with this exit code

    CloseHandle(process);
    CloseHandle(thread);
}

TEST(InjectingTest, InjectionNoQuitInit) {
    // Verify lib can inject with a init function with null parameters

    HANDLE process, thread;
    spawn(process, thread);
    EXPECT_NO_THROW(InjectLib::InjectDLL(process, thread, INJECT_LIB.data(), "InitNoQuit"));
    ResumeThread(thread);

    DWORD res = WaitForSingleObject(process, INFINITE);
    DWORD exitCode = NO_ERROR;
    res = GetExitCodeProcess(process, &exitCode);
    EXPECT_EQ(0, exitCode); // expect regular exit from process

    CloseHandle(process);
    CloseHandle(thread);
}

TEST(InjectingTest, InjectionSkipInit) {
    // verify the skip-on-missing mechanism for init function works

    HANDLE process, thread;
    spawn(process, thread);
    EXPECT_NO_THROW(InjectLib::InjectDLL(process, thread, INJECT_LIB.data(), "__InitInvalid", nullptr, 0, true));
    ResumeThread(thread);

    DWORD res = WaitForSingleObject(process, INFINITE);
    DWORD exitCode = NO_ERROR;
    res = GetExitCodeProcess(process, &exitCode);
    EXPECT_EQ(NOERROR, exitCode);

    CloseHandle(process);
    CloseHandle(thread);
}

int main(int argc, char** argv) {
    fs::path filePath(winapi::wide::getModuleFileName(nullptr));
    fs::current_path(filePath.parent_path());

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
