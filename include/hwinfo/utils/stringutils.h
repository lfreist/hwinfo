// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>

/**
 * remove all white spaces (' ', '\t', '\n') from start and end of input
 * inplace!
 * @param input
 */
inline void strip(std::string& input) {
  size_t start_index = 0;
  while (true) {
    char c = input[start_index];
    if (c != ' ' && c != '\t' && c != '\n') { break; }
    start_index++;
  }
  size_t end_index = input.size() - 1;
  while (true) {
    char c = input[end_index];
    if (c != ' ' && c != '\t' && c != '\n') { break; }
    end_index--;
  }
  if (end_index <= start_index) {
    input.assign("");
    return;
  }
  input.assign(input.begin()+start_index, input.begin()+end_index+1);
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
  if (occ <= index) { return ""; }

  std::string::size_type start_index = 0;
  while (true) {
    if (index == 0) { break; }
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
inline std::string wstring_to_string() {
  return "";
}