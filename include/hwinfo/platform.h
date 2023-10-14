#pragma once

#if defined(unix) || defined(__unix) || defined(__unix__)
#define HWINFO_UNIX
#endif
#if defined(__APPLE__)
#define HWINFO_APPLE
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define HWINFO_WINDOWS
#endif

#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(_M_X64)
#define HWINFO_X86_64
#elif defined(__i386__) || defined(_M_IX86)
#define HWINFO_X86_32
#endif
#if defined(HWINFO_X86_64) || defined(HWINFO_X86_32)
#define HWINFO_X86
#endif

// macro definitions for C++ > 11 features if compiled with C++ > 11
#if __cplusplus >= 201703L
#define HWI_NODISCARD [[nodiscard]]
#else
#define HWI_NODISCARD
#endif