# Copyright (C) 2025 ETH Zurich. All rights reserved.
#
# Author: Cedric Hirschi, ETH Zurich
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# /// script
# dependencies = [
#   "zeroconf",
# ]
# ///
import sys
import socket
import json

from wulpus.finder import WulpusServiceFinder


message = None


def bytes_to_ip(ip_bytes):
    return ".".join(map(str, ip_bytes))


def send_message(ip, port, message):
    # send message to ip:port via TCP
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((ip, port))
        s.send(message.encode())
        s.close()
    except ConnectionRefusedError:
        print(f"Connection to {ip}:{port} refused")
        s.close()


if __name__ == "__main__":

    argument_1 = sys.argv[1] if len(sys.argv) > 1 else None
    argument_2 = sys.argv[2] if len(sys.argv) > 2 else None

    if argument_1:
        service_type = argument_1
    else:
        print("Usage: python mdns_disco.py <service_type>")
        sys.exit(1)

    if argument_2:
        message = sys.argv[2]

    finder = WulpusServiceFinder(service_type)
    print(f"Finding service '{service_type}'")
    finder.find()
    devices = finder.get_devices()
    print(f"Found {len(devices)} devices")

    with open("devices.json", "w") as f:
        data = {"devices": []}

        for device in devices:
            data["devices"].append({"name": device.name, "server": device.server, "ip": device.ip, "port": device.port})

        f.write(json.dumps(data, indent=4))

    print("Done")
