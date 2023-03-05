#pragma once
#include <string>

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

// This function replaces the std::string::starts_with function only available in C++20 and above
template <typename string_type, typename prefix_type>
inline bool starts_with(const string_type& str, const prefix_type& prefix) {
#ifdef __cpp_lib_starts_ends_with
	return str.starts_with(prefix);
#else
	return str.rfind(prefix, 0) == 0;
#endif
}
