// Preprocessor Machinary for static and shared libraries.
// Exports the DLLEXPORT macro which should be used.
#pragma once

// If COMMON_DLL is defined, export/import synbols as needed.
// if not, noop.
#if defined(COMMON_DLL)
#if COMMON_DLL_BUILD
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif
#else
#define DLLEXPORT
#endif
