// Provides windows typedefs and forward declareations.
#pragma once
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
