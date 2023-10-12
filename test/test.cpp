#include <hwinfo/utils/wmi_wrapper.h>
#include <hwinfo/CPU.h>

#include <iostream>

int main() {
    auto res = hwinfo::utils::WMI::get_component<hwinfo::CPU>();

    for (auto r : res) {
        std::cout << r.modelName() << std::endl;
        std::cout << r.vendor() << std::endl;
    }

    return 0;
}