#pragma once
#include "usvfs/dllimport.h"
#include <Unknwn.h>
#include <common/sane_windows.h>

namespace usvfs {

namespace hooks {

DLLEXPORT HRESULT WINAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid,
                                          LPVOID* ppv);

DLLEXPORT HRESULT WINAPI CoCreateInstanceEx(REFCLSID rclsid, IUnknown* punkOuter, DWORD dwClsCtx,
                                            COSERVERINFO* pServerInfo, DWORD dwCount, MULTI_QI* pResults);

} // namespace hooks

} // namespace usvfs
