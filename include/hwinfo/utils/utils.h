//
// Created by leon- on 23/06/2023.
//

#pragma once

#include <string>
#include <vector>

namespace utils {

template <typename T>
T get_value(const std::vector<T>& data, size_t index);

template <>
inline std::string get_value<std::string>(const std::vector<std::string>& data, size_t index) {
  if (data.size() < index) {
    return "<unknown>";
  } else {
    return data[index];
  }
}

template <>
inline int64_t get_value<int64_t>(const std::vector<int64_t>& data, size_t index) {
  if (data.size() < index) {
    return -1;
  } else {
    return data[index];
  }
}

}  // namespace utils