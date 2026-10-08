// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Internal file reading helpers returning hwinfo::result. Not part of the public API.

#pragma once

#include <hwinfo/error.h>

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "strings.h"

namespace hwinfo::internal {

inline error file_error(const std::filesystem::path& path) {
  const int err = errno;
  return error{std::error_code(err != 0 ? err : EIO, std::generic_category()), "cannot read " + path.string()};
}

// Reads the entire file.
inline result<std::string> read_file(const std::filesystem::path& path) {
  errno = 0;
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return std::unexpected(file_error(path));
  }
  std::string content(std::istreambuf_iterator<char>{file}, {});
  if (file.bad()) {
    return std::unexpected(file_error(path));
  }
  return content;
}

// Reads the first line of the file, trimmed. Typical for sysfs attributes.
inline result<std::string> read_line(const std::filesystem::path& path) {
  errno = 0;
  std::ifstream file(path);
  if (!file) {
    return std::unexpected(file_error(path));
  }
  std::string line;
  if (!std::getline(file, line) && file.bad()) {
    return std::unexpected(file_error(path));
  }
  return std::string(trim(line));
}

// Reads the first line of the file as a number.
template <typename T>
result<T> read_number(const std::filesystem::path& path, int base = 10) {
  return read_line(path).and_then([&](const std::string& s) {
    return parse<T>(s, base).transform_error([&](const error& e) { return error{e.code(), path.string()}; });
  });
}

// Reads a sysfs-like attribute: std::nullopt if it is missing, empty or unreadable.
inline std::optional<std::string> read_attribute(const std::filesystem::path& path) {
  auto line = read_line(path);
  return line ? non_empty(*line) : std::nullopt;
}

template <typename T>
std::optional<T> read_number_attribute(const std::filesystem::path& path, int base = 10) {
  auto value = read_number<T>(path, base);
  return value ? std::optional<T>(*value) : std::nullopt;
}

}  // namespace hwinfo::internal
