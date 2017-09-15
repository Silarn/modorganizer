// Handles all the messy Windows.h shit
// For convienence, provides the windows typedefs if the NO_WINDOWSH macro is defined.
// Header guard intentionally missing so that NO_WINDOWSH can be used properly.
// NO_WINDOWSH should be redefined after each intended use, as the header will undefine it.
#if 0
#pragma region
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#pragma endregion NOShit
#endif

#if !defined(NO_WINDOWSH)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#undef NO_WINDOWSH

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
#endif
