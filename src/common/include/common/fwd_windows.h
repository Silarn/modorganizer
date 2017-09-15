// Provides windows typedefs and forward declareations.
#pragma once
// Only if Windows.h isn't already included.
#if !defined(WINAPI)

using WCHAR = wchar_t;
using CHAR = char;
using LPWSTR = WCHAR*;
using LPSTR = CHAR*;
using LPCSTR = const LPSTR;
using LPCWSTR = const LPWSTR;
using PCWSTR = LPCWSTR;
using PCSTR = LPCSTR;
#if UNICODE
using LPCTSTR = LPCWSTR;
using LPTSTR = LPWSTR;
using PCTSTR = PCWSTR;
using TBYTE = WCHAR;
#else
using LPCTSTR = LPCSTR;
using LPTSTR = LPSTR;
using PCTSTR = PCSTR;
using TBYTE = CHAR;
#endif
using TCHAR = TBYTE;

using LONG = long;
using SHORT = short;
using INT = int;
using LPVOID = void*;
using LPCSTR = const CHAR*;
using BYTE = unsigned char;
using PBYTE = BYTE*;
using LPBYTE = PBYTE;
using ULONG = unsigned long;
using BOOL = int;
#define TRUE 1
#define FALSE 0
using DWORD = ULONG;
#define WINAPI __stdcall

#if !defined(_M_IX86)
using LONGLONG = __int64;
#else
using LONGLONG = double;
#endif
#else
#pragma message ("Warning: This isn't very nice for fwd_windows.h")
#endif
