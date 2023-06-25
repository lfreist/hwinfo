"""
Copyright (R) 2023, Leon Freist
Author: Leon Freist <freist.leon@gmail.com>

This script is used to build a compile time PCI ID mapper in C++.
We use the pci.ids file (https://pci-ids.ucw.cz/v2.2/pci.ids) licenced under BSD3.
"""

import sys
from dataclasses import dataclass, field
from typing import Dict


@dataclass
class PCIDevice:
    device_id: str = field(default_factory=str)
    device_name: str = field(default_factory=str)
    subsystems: Dict[str, str] = field(default_factory=dict)


@dataclass
class PCIVendor:
    vendor_id: str = field(default_factory=str)
    vendor_name: str = field(default_factory=str)
    devices: Dict[str, PCIDevice] = field(default_factory=dict)


class Parser:
    def __init__(self, pci_ids_path: str):
        self.in_path = pci_ids_path

    def parse(self):
        pass

    def get_vendors(self) -> PCIVendor:
        for line in self.read_lines():
            line = line.strip()
            v_id, v_name = line.split("  ")
            vendor: PCIVendor = PCIVendor(v_id, v_name)
            vendor.devices = self.__next_devices()
            yield vendor

    def __next_devices(self) -> Dict[str, PCIDevice]:
        device_done: bool = False
        devices: Dict[str, PCIDevice] = {}
        device_id: str
        for line in self.read_lines():
            if not line.startswith('\t') and not line.startswith("\t\t"):
                # line is a new vendor
                # TODO: we have a problem here... line is a vendor now and we cannot get this line again in the
                #       get_vendors() method again. We need to parse the content of the pci.ids file in another way!
                break
            if line.startswith('\t'):
                line = line.strip()
                device_id, d_name = line.split("  ")
                devices[device_id] = PCIDevice(device_id, d_name)
                devices[device_id].subsystems = self.__get_subsystems()

    def __get_subsystems(self) -> Dict[str, str]:
        pass

    def read_lines(self):
        with open(self.in_path) as f:
            for line in f.readlines():
                # skip all nonsense lines (comments and empty lines)
                if line.startswith("#") or len(line) == 0 or line == '\n':
                    continue
                yield line


if __name__ == "__main__":
    pass
