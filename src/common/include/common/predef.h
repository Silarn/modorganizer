// Useful Preprocessor Machinary for
#pragma once

#if defined(_MSC_VER)

#if defined(_M_AMD64)
#define COMMON_IS_64 1
#else
#define COMMON_IS_64 0
#endif

#if defined(_M_IX86)
#define COMMON_IS_86 1
#else
#define COMMON_IS_86 0
#endif

#if defined(_DEBUG)
#define COMMON_IS_DEBUG 1
#else
#define COMMON_IS_DEBUG 0
#endif

#endif
