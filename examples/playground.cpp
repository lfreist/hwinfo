#include <iostream>
#include <filesystem>
#include <fstream>

#define fs std::filesystem

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) return "";
  std::string line;
  std::getline(file, line);
  // Trim whitespace
  line.erase(line.find_last_not_of(" \n\r\t") + 1);
  return line;
}

void getAllDisks() {
  std::uint32_t current_id = 0;

  for (const auto& entry : fs::directory_iterator("/sys/class/block")) {
    std::string name = entry.path().filename().string();

    // Skip partitions (e.g., sda1), loop devices, and virtual devices
    if (fs::exists(entry.path() / "partition") || name.find("loop") == 0 || name.find("ram") == 0) {
      continue;
    }

    std::cout << current_id++ << std::endl;

    // Basic Disk Info
    std::cout << read_file(entry.path() / "device/model") << std::endl;
    std::cout << read_file(entry.path() / "device/vendor") << std::endl;

    // Serial number often requires a different path depending on driver
    std::cout << read_file(entry.path() / "device/serial") << std::endl;

    // Size in blocks (usually 512 bytes)
    std::cout << read_file(entry.path() / "size") << std::endl;

    // Determine Interface
    std::string subsystem = fs::canonical(entry.path() / "device").string();
    std::cout << subsystem << std::endl;

    // Find Volumes (Partitions) associated with this disk
    for (const auto& sub_entry : fs::directory_iterator(entry.path())) {
      if (fs::exists(sub_entry.path() / "partition")) {
        std::cout << "/dev/" << sub_entry.path().filename().string() << std::endl;
      }
    }
  }
}

int main() {
  getAllDisks();
}