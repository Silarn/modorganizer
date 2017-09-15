#pragma once
#include <common/fwd_windows.h>

static DWORD tlsIndex = (DWORD)-1;

LPVOID InitTLS();

void WINAPI StoreTLS(DWORD value);

DWORD WINAPI GetTLS();

class HookLock {

  public:
    static const DWORD FIND_FILE_GROUP = 0x00000001;
    static const DWORD GET_ATTRIBUTES_GROUP = 0x00000002;
    static const DWORD GET_PROFILESTRING_GROUP = 0x00000004;
    static const DWORD SH_FILEOPERATION_GROUP = 0x00000008;
    static const DWORD GET_FILEVERSION_GROUP = 0x00000010;
    static const DWORD GET_MODULENAME_GROUP = 0x00000020;

  public:
    HookLock(DWORD group);

    ~HookLock();

    static bool isLocked(DWORD group);

  private:
    bool m_Owner;
    DWORD m_Group;
};
