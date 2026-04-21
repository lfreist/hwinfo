// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <algorithm>
#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace hwinfo {
namespace utils {

/**
 * Replaces an occurence once in the entire string. Stops at first match
 *
 * @param input The input string
 * @param from The token to match
 * @param to The token to use as a replace with matched ones
 */
inline bool replaceOnce(std::string& input, const std::string& from, const std::string& to) {
  size_t start_pos = input.find(from);

  if (start_pos == std::string::npos) return false;

  input.replace(start_pos, from.length(), to);

  return true;
}

/**
 * Replaces all occurences in the entire string.
 *
 * @param input The input string
 * @param from The token to match
 * @param to The token to use as a replace with matched ones
 */
inline void replaceAll(std::string& input, const char from, const char to) {
  std::replace(input.begin(), input.end(), from, to);
}

/**
 * remove all white spaces (' ', '\t', '\n') from start and end of input
 * inplace!
 * @param input
 */
inline void strip(std::string& input) {
  if (input.empty()) {
    return;
  }
  // optimization for input size == 1
  if (input.size() == 1) {
    if (input[0] == ' ' || input[0] == '\t' || input[0] == '\n') {
      input.clear();
      return;
    } else {
      return;
    }
  }
  size_t start_index = input.find_first_not_of(" \t\n");
  if (start_index == std::string::npos) {
    input.clear();
    return;
  }
  size_t end_index = input.find_last_not_of(" \t\n");
  input.assign(input.begin() + start_index, input.begin() + end_index + 1);
}

/**
 * Count occurrences of a substring in input
 * @param input
 * @param substring
 * @return
 */
inline unsigned count_substring(const std::string& input, const std::string& substring) {
  unsigned occurrences = 0;
  std::string::size_type shift = 0;
  while ((shift = input.find(substring, shift)) != std::string::npos) {
    occurrences++;
    shift += substring.size();
  }
  return occurrences;
}

/**
 * Split input string at delimiter and return result
 * @param input
 * @param delimiter
 * @return
 */
inline std::vector<std::string> split(const std::string& input, const std::string& delimiter) {
  std::vector<std::string> result;
  size_t shift = 0;
  while (true) {
    size_t match = input.find(delimiter, shift);
    result.emplace_back(input.substr(shift, match - shift));
    if (match == std::string::npos) {
      break;
    }
    shift = match + delimiter.size();
  }
  return result;
}

/**
 * Split input string at delimiter (char) and return result
 * @param input
 * @param delimiter
 * @return
 */
inline std::vector<std::string> split(const std::string& input, const char delimiter) {
  std::vector<std::string> result;
  size_t shift = 0;
  while (true) {
    size_t match = input.find(delimiter, shift);
    if (match == std::string::npos) {
      break;
    }
    result.emplace_back(input.substr(shift, match - shift));
    shift = match + 1;
  }
  return result;
}

/**
 * split input at delimiter and return substring at position index.
 * index can be negative, where -1 is the last occurrence.
 * @param input
 * @param delimiter
 * @param index
 * @return
 */
inline std::string split_get_index(const std::string& input, const std::string& delimiter, int index) {
  unsigned occ = count_substring(input, delimiter) + 1;
  index = index < 0 ? static_cast<int>(occ + index) : index;
  if (static_cast<int>(occ) <= index) {
    return "";
  }

  std::string::size_type start_index = 0;
  while (true) {
    if (index == 0) {
      break;
    }
    start_index = input.find(delimiter, start_index) + delimiter.size();
    index--;
  }
  std::string::size_type end_index = input.find(delimiter, start_index);
  if (end_index == std::string::npos) {
    return {input.begin() + static_cast<int64_t>(start_index), input.end()};
  }
  return {input.begin() + static_cast<int64_t>(start_index), input.begin() + static_cast<int64_t>(end_index)};
}

/**
 * Convert windows wstring to string
 * @return
 */
inline std::string wstring_to_string() { return ""; }

/**
 * Convert wstring to string
 * @return
 */
inline std::string wstring_to_std_string(const std::wstring& ws) {
  if (ws.empty()) return std::string();

#ifdef _WIN32
  // --------------------------------------------------------------------
  // Windows / MinGW: wchar_t is 16-bit UTF-16 (with surrogate pairs)
  // Manually convert UTF-16 to UTF-8.
  // --------------------------------------------------------------------
  std::string utf8;
  utf8.reserve(ws.size() * 3);  // Worst case: 3 bytes per code unit

  for (size_t i = 0; i < ws.size(); ++i) {
    uint32_t code_point = static_cast<uint32_t>(ws[i]);

    // Check for high surrogate (0xD800 - 0xDBFF)
    if (code_point >= 0xD800 && code_point <= 0xDBFF) {
      // High surrogate: expect a low surrogate next
      if (i + 1 < ws.size()) {
        uint32_t low = static_cast<uint32_t>(ws[i + 1]);
        if (low >= 0xDC00 && low <= 0xDFFF) {
          // Valid surrogate pair
          code_point = 0x10000 + ((code_point - 0xD800) << 10) + (low - 0xDC00);
          ++i;  // Skip low surrogate
        } else {
          // Invalid sequence: treat high surrogate as isolated (fallback to replacement)
          code_point = 0xFFFD;
        }
      } else {
        // Truncated high surrogate at end of string
        code_point = 0xFFFD;
      }
    } else if (code_point >= 0xDC00 && code_point <= 0xDFFF) {
      // Isolated low surrogate: invalid, replace
      code_point = 0xFFFD;
    }

    // Encode code_point into UTF-8
    if (code_point <= 0x7F) {
      utf8.push_back(static_cast<char>(code_point));
    } else if (code_point <= 0x7FF) {
      utf8.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
      utf8.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else if (code_point <= 0xFFFF) {
      utf8.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
      utf8.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
      utf8.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else if (code_point <= 0x10FFFF) {
      utf8.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
      utf8.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
      utf8.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
      utf8.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else {
      // Invalid code point (beyond Unicode range)
      // Replace with U+FFFD
      utf8.push_back(0xEF);
      utf8.push_back(0xBF);
      utf8.push_back(0xBD);
    }
  }
  utf8.reserve(utf8.size());
  return utf8;

#else
  std::string str_locale = std::setlocale(LC_ALL, NULL);

  std::setlocale(LC_ALL, ".UTF-8");

  const wchar_t* wch_src = ws.c_str();

  size_t n_dest_size = std::wcstombs(NULL, wch_src, 0) + 1;
  char* ch_dest = new char[n_dest_size];
  std::memset(ch_dest, 0, n_dest_size);
  std::wcstombs(ch_dest, wch_src, n_dest_size);
  std::string result_text = ch_dest;
  delete[] ch_dest;

  std::setlocale(LC_ALL, str_locale.c_str());
  return result_text;
#endif
}

/**
 * Replace the std::string::starts_with function only available in C++20 and above.
 * @param str
 * @param prefix
 * @return
 */
template <typename string_type, typename prefix_type>
inline bool starts_with(const string_type& str, const prefix_type& prefix) {
#ifdef __cpp_lib_starts_ends_with
  return str.starts_with(prefix);
#else
  return str.rfind(prefix, 0) == 0;
#endif
}

}  // namespace utils
}  // namespace hwinfo
