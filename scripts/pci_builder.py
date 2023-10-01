"""
Copyright (R) 2023, Leon Freist
Author: Leon Freist <freist.leon@gmail.com>

This script is used to build a compile time PCI ID mapper in C++.
We use the pci.ids file (https://pci-ids.ucw.cz/v2.2/pci.ids) licenced under BSD3.
"""

import sys
from dataclasses import dataclass, field
from typing import Generator, Optional
from collections import OrderedDict


@dataclass
class PCIDevice:
    id: str = field(default_factory=str)
    name: str = field(default_factory=str)
    subsystems: OrderedDict[str, str] = field(default_factory=OrderedDict)

    def __repr__(self):
        return f"{'{'}{self.id}, {self.name}"


@dataclass
class PCIVendor:
    id: str = field(default_factory=str)
    name: str = field(default_factory=str)
    devices: OrderedDict[str, PCIDevice] = field(default_factory=OrderedDict)


class PCIParser:

    def __init__(self, pci_ids_path: str):
        self.in_path = pci_ids_path

    def parse(self) -> Generator[PCIVendor, None, None]:
        last_vendor: Optional[PCIVendor] = None
        last_device_id: str = ""
        for line in self.read_lines():
            processed_line = line.strip()
            _id, _info = processed_line.split("  ", maxsplit=1)
            if line.startswith('\t'):
                # device or subsystem
                if line.startswith("\t\t"):
                    # subsystem
                    last_vendor.devices[last_device_id].subsystems[_id] = _info
                else:
                    # device
                    last_vendor.devices[_id] = PCIDevice(_id, _info)
                    last_device_id = _id
            else:
                if last_vendor:
                    yield last_vendor
                # vendor
                last_vendor = PCIVendor(_id, _info)

    def read_lines(self) -> Generator[str, None, None]:
        with open(self.in_path) as f:
            for line in f.readlines():
                # skip all nonsense lines (comments and empty lines)
                if line.startswith("#") or len(line) == 0 or line == '\n':
                    continue
                yield line


if __name__ == "__main__":
    import os
    path = "pci.ids"
    if not os.path.isfile(path):
        path = "scripts/pci.ids"
    if not os.path.isfile(path):
        path = "hwinfo/scripts/pci.ids"
    else:
        print("pci.ids file could not be found")
        exit(1)
    parser = PCIParser(path)
    for vendor in parser.parse():
        print(f"{'{'}{vendor.id}")

