"""
Copyright (R) 2023, Leon Freist

parse drm file to json
"""

import sys
import json
from typing import Iterable


def read_drm_file(path: str) -> Iterable[str]:
    """
    Read drm file line by line while skipping comments (starting with '#')
    :param path:
    :return:
    """
    with open(path, "r") as f:
        for line in f.readlines():
            if line.startswith("#") or line.strip() == "":
                continue
            yield line


def parse_drm_to_json(drm_path: str, json_path: str) -> None:
    """
    Read drm file and output corresponding json file.
    DRM format:
        vendor  vendor_name
            device  device_name				        <-- single tab
        	    subvendor subdevice  subsystem_name	<-- two tabs
    JSON format:
        {
            'vendor device': {
                "vendor_name": vendor_name,
                "device_name": device_name,
                "subvendors": {
                    'subvendor subdevice': "subsystem_name",
                    ...
                }
            },
            ...
        }
    :param drm_path:
    :param json_path:
    :return:
    """
    json_data: dict = {}
    last_vendor: dict = {
        "id": "",
        "name": ""
    }
    last_device_id = ""
    for line in read_drm_file(drm_path):
        if line.startswith("\t\t"):
            line = line.strip()
            id = line.split("  ")[0]
            name = line.split("  ")[1]
            json_data[f"{last_vendor['id']} {last_device_id}"]["sub_devices"][id] = name
        elif line.startswith('\t'):
            line = line.strip()
            device_id = line.split("  ")[0]
            device_name = line.split("  ")[1]
            json_data[f"{last_vendor['id']} {device_id}"] = {
                "vendor_name": last_vendor["name"],
                "device name": device_name,
                "sub_devices": {}
            }
            last_device_id = device_id
        else:
            line = line.strip()
            vendor_id = line.split("  ")[0]
            vendor_name = line.split("  ")[1]
            last_vendor = {
                "id": vendor_id,
                "name": vendor_name
            }
    with open(json_path, "w") as f:
        json.dump(json_data, f, indent=4)


if __name__ == "__main__":
    json_file = "../assets/drm.json"
    if len(sys.argv) == 1:
        drm_file = "/dev/stdin"
    elif len(sys.argv) == 2:
        drm_file = sys.argv[1]
    else:
        print(f"Usage: python3 {__file__} [path/to/drm]")
        print("If no drm file is provided, stdin is read instead.")
        exit(1)
    parse_drm_to_json(drm_file, json_file)
