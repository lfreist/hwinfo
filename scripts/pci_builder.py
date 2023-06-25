"""
Copyright (R) 2023, Leon Freist
Author: Leon Freist <freist.leon@gmail.com>

This script is used to build a compile time PCI ID mapper in C++.
We use the pci.ids file (https://pci-ids.ucw.cz/v2.2/pci.ids) licenced under BSD3.
"""

import sys
from dataclasses import dataclass, field
from typing import Generator
from collections import OrderedDict


@dataclass
class PCIDevice:
    device_id: str = field(default_factory=str)
    device_name: str = field(default_factory=str)
    subsystems: OrderedDict[str, str] = field(default_factory=OrderedDict)


@dataclass
class PCIVendor:
    vendor_id: str = field(default_factory=str)
    vendor_name: str = field(default_factory=str)
    devices: OrderedDict[str, PCIDevice] = field(default_factory=OrderedDict)


class PCIParser:

    def __init__(self, pci_ids_path: str):
        self.in_path = pci_ids_path

    def parse(self) -> OrderedDict[str, PCIVendor]:
        result: OrderedDict[str, PCIVendor] = OrderedDict()
        last_vendor_id: str = ""
        last_device_id: str = ""
        for line in self.read_lines():
            processed_line = line.strip()
            _id, _info = processed_line.split("  ")
            if line.startswith('\t'):
                # device or subsystem
                if line.startswith("\t\t"):
                    # subsystem
                    result[last_vendor_id].devices[last_device_id].subsystems[_id] = _info
                else:
                    # device
                    result[last_vendor_id].devices[_id] = PCIDevice(_id, _info)
                    last_device_id = _id
            else:
                # vendor
                result[_id] = PCIVendor(_id, _info)
                last_vendor_id = _id
        return result

    def read_lines(self) -> Generator[str, None, None]:
        with open(self.in_path) as f:
            for line in f.readlines():
                # skip all nonsense lines (comments and empty lines)
                if line.startswith("#") or len(line) == 0 or line == '\n':
                    continue
                yield line


if __name__ == "__main__":
    pass
