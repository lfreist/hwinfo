// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

std::string exec(std::string& command) {
  std::string output;
  std::vector<char> buffer(0x80);
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
  if (!pipe) {
    return "";
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    output.append(buffer.data());
  }
  return output;
}