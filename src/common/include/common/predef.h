// Useful Preprocessor Machinary for
#pragma once

#ifdef _MSC_VER

#if _M_AMD64 == 100
#define COMMON_IS_64 1
#else
#define COMMON_IS_64 0
#endif

#if _M_IX86 == 600
#define COMMON_IS_86 1
#else
#define COMMON_IS_86 0
#endif

#ifdef _DEBUG
#define COMMON_IS_DEBUG 1
#else
#define COMMON_IS_DEBUG 0
#endif

#endif
