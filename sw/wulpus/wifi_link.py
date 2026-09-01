"""
Copyright (C) 2025 ETH Zurich. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

SPDX-License-Identifier: Apache-2.0

TCP communication link for WULPUS PRO Wi-Fi hosts.
"""

import logging
import socket
import struct
import time
from dataclasses import dataclass
from enum import IntEnum
from typing import Any, List, Optional, Tuple

import numpy as np

from .wifi_discovery import WulpusProWiFiDevice, WulpusProWiFiDiscovery

logger = logging.getLogger(__name__)

HEADER_MAGIC = b"wulpus"
HEADER_LENGTH = 9
MAX_PAYLOAD_LENGTH = 65535


class WulpusProWiFiCommand(IntEnum):
    """Commands used by the WULPUS PRO TCP and USB CDC protocol."""

    SET_CONFIG = 0x57
    GET_DATA = 0x58
    PING = 0x59
    PONG = 0x5A
    RESET = 0x5B
    RESET_MSP = 0x63
    CLOSE = 0x5C
    START_RX = 0x5D
    STOP_RX = 0x5E
    BUSY = 0x5F
    GET_STATUS = 0x60
    STATUS = 0x61
    CLEAR_STATUS = 0x62

    def __str__(self) -> str:
        return f"{self.__class__.__name__}.{self.name}"

    def __repr__(self) -> str:
        return str(self)


class WulpusProWiFiError(Exception):
    """Base exception for WULPUS PRO Wi-Fi communication failures."""


class WulpusProWiFiTimeout(WulpusProWiFiError):
    """Raised when a required protocol operation times out."""


class WulpusProWiFiDisconnected(WulpusProWiFiError):
    """Raised when the remote Wi-Fi host closes the TCP connection."""


class WulpusProWiFiProtocolError(WulpusProWiFiError):
    """Raised when a malformed or unexpected protocol packet is received."""


class WulpusProWiFiBusy(WulpusProWiFiProtocolError):
    """Raised when the device cannot accept a command in its current state."""


@dataclass(frozen=True)
class WulpusProWiFiHeader:
    command: WulpusProWiFiCommand
    length: int
    magic: str = "wulpus"

    def __getitem__(self, key: str) -> Any:
        # Preserve the dictionary-style access used by older notebooks.
        return getattr(self, key)


@dataclass(frozen=True)
class WulpusProFrame:
    samples: np.ndarray
    acquisition_number: int
    tx_rx_id: int


@dataclass(frozen=True)
class WulpusProStatus:
    version: int
    error_flags: int
    buffer_overflow_count: int
    data_ready_count: int
    completed_spi_count: int
    transmitted_frame_count: int
    discarded_frame_count: int
    spi_error_count: int
    link_error_count: int
    current_buffer_usage: int
    maximum_buffer_usage: int


class WulpusProWiFiLink:
    """Discover and communicate with an ESP32-based WULPUS PRO host."""

    def __init__(
        self,
        service_name: str = "wulpus_pro",
        service_type: str = "tcp",
        port: int = 2121,
    ) -> None:
        self.service_name = service_name
        self.service_type = service_type
        self.port = port
        self.discovery = WulpusProWiFiDiscovery(service_name, service_type)
        # Kept as an attribute for compatibility with existing callers.
        self.scanner = self.discovery
        self.device: Optional[WulpusProWiFiDevice] = None
        self.sock: Optional[socket.socket] = None
        self.backlog = b""
        self.acq_length = 400
        self.rx_enabled = False

    @property
    def connected(self) -> bool:
        return self.sock is not None

    def __enter__(self) -> "WulpusProWiFiLink":
        return self

    def __exit__(self, exc_type: Any, exc_value: Any, traceback: Any) -> None:
        self.close()

    def get_available(self, timeout: float = 5.0) -> List[WulpusProWiFiDevice]:
        devices = self.discovery.find(timeout)
        if devices:
            return devices

        # On some Windows configurations multicast browsing is blocked even
        # though the ESP32's .local hostname can still be resolved.
        hostname = f"{self.service_name}.local"
        try:
            address_info = socket.getaddrinfo(
                hostname, self.port, type=socket.SOCK_STREAM
            )
        except OSError:
            logger.warning(
                "Could not discover %s through mDNS or hostname resolution",
                hostname,
            )
            return []

        addresses = sorted(
            {entry[4][0] for entry in address_info},
            key=lambda address: (":" in address, address),
        )
        if not addresses:
            return []

        device = WulpusProWiFiDevice(
            name=self.service_name,
            server=hostname,
            ip=addresses[0],
            port=self.port,
        )
        self.discovery.devices = [device]
        logger.info("Resolved WULPUS PRO Wi-Fi device as %s", device)
        return [device]

    def open(self, device: Optional[WulpusProWiFiDevice] = None) -> bool:
        if self.connected:
            logger.warning("WULPUS PRO Wi-Fi link is already open")
            return True

        if device is None:
            if not self.discovery.devices:
                logger.error("No discovered WULPUS PRO Wi-Fi device is available")
                return False
            device = self.discovery.devices[0]

        sock = socket.socket(socket.AF_INET6 if ":" in device.ip else socket.AF_INET)
        sock.settimeout(5.0)
        try:
            sock.connect((device.ip, device.port))
        except OSError as exc:
            logger.error("Could not connect to %s: %s", device, exc)
            sock.close()
            return False

        self.sock = sock
        self.device = device
        self.backlog = b""
        self.rx_enabled = False
        logger.info("Connected to %s", device)
        return True

    def connect_first_available(self, timeout: float = 5.0) -> bool:
        devices = self.get_available(timeout)
        return bool(devices) and self.open(devices[0])

    def _require_socket(self) -> socket.socket:
        if self.sock is None:
            raise WulpusProWiFiDisconnected("WULPUS PRO Wi-Fi link is not open")
        return self.sock

    @staticmethod
    def _encode_command(
        command: WulpusProWiFiCommand, data: bytes = b""
    ) -> bytes:
        if len(data) > MAX_PAYLOAD_LENGTH:
            raise ValueError("Command payload is too large")
        return HEADER_MAGIC + struct.pack("<BH", int(command), len(data)) + data

    @staticmethod
    def _decode_header(header_bytes: bytes) -> WulpusProWiFiHeader:
        if len(header_bytes) != HEADER_LENGTH:
            raise WulpusProWiFiProtocolError(
                f"Expected a {HEADER_LENGTH}-byte header, got {len(header_bytes)} bytes"
            )
        magic, command_value, length = struct.unpack("<6sBH", header_bytes)
        if magic != HEADER_MAGIC:
            raise WulpusProWiFiProtocolError(
                f"Invalid protocol magic {magic!r}; expected {HEADER_MAGIC!r}"
            )
        try:
            command = WulpusProWiFiCommand(command_value)
        except ValueError as exc:
            raise WulpusProWiFiProtocolError(
                f"Unknown command ID 0x{command_value:02X}"
            ) from exc
        return WulpusProWiFiHeader(command=command, length=length)

    def _recv_exact(self, length: int, timeout: Optional[float] = None) -> bytes:
        sock = self._require_socket()
        if length < 0:
            raise ValueError("Receive length must not be negative")

        original_timeout = sock.gettimeout()
        deadline = None if timeout is None else time.monotonic() + timeout
        received = bytearray()
        try:
            while len(received) < length:
                if deadline is not None:
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        raise WulpusProWiFiTimeout(
                            f"Timed out after receiving {len(received)} of {length} bytes"
                        )
                    sock.settimeout(remaining)
                try:
                    chunk = sock.recv(length - len(received))
                except socket.timeout as exc:
                    raise WulpusProWiFiTimeout(
                        f"Timed out after receiving {len(received)} of {length} bytes"
                    ) from exc
                except OSError as exc:
                    self._discard_socket()
                    raise WulpusProWiFiDisconnected(str(exc)) from exc
                if not chunk:
                    self._discard_socket()
                    raise WulpusProWiFiDisconnected(
                        f"Connection closed after receiving {len(received)} of {length} bytes"
                    )
                received.extend(chunk)
        finally:
            if self.sock is sock:
                sock.settimeout(original_timeout)
        return bytes(received)

    def receive_command(
        self, timeout: Optional[float] = None
    ) -> Tuple[WulpusProWiFiHeader, bytes]:
        return self._receive_packet(timeout)

    def _receive_packet(
        self, timeout: Optional[float] = None
    ) -> Tuple[WulpusProWiFiHeader, bytes]:
        """Receive one complete framed packet through the shared backlog."""
        sock = self._require_socket()
        original_timeout = sock.gettimeout()
        deadline = None if timeout is None else time.monotonic() + timeout
        try:
            while True:
                packet = self._extract_packet_from_backlog()
                if packet is not None:
                    return packet

                remaining = None if deadline is None else deadline - time.monotonic()
                if remaining is not None and remaining <= 0:
                    raise WulpusProWiFiTimeout("Timed out waiting for a packet")
                sock.settimeout(original_timeout if remaining is None else remaining)
                try:
                    chunk = sock.recv(4096)
                except socket.timeout as exc:
                    raise WulpusProWiFiTimeout(
                        "Timed out waiting for a packet"
                    ) from exc
                except OSError as exc:
                    self._discard_socket()
                    raise WulpusProWiFiDisconnected(str(exc)) from exc
                if not chunk:
                    self._discard_socket()
                    raise WulpusProWiFiDisconnected("Connection closed")
                self.backlog += chunk
        finally:
            if self.sock is sock:
                sock.settimeout(original_timeout)

    def send_command(
        self,
        command: WulpusProWiFiCommand,
        data: bytes = b"",
        expected_response: Optional[WulpusProWiFiCommand] = None,
        timeout: Optional[float] = None,
        receive: Optional[bool] = None,
    ) -> Tuple[Optional[WulpusProWiFiHeader], Optional[bytes]]:
        # The firmware acknowledges every command by echoing its header with an
        # empty payload. The old API exposed receive=, so retain it as an
        # explicit compatibility override.
        if receive is False:
            expected_response = None
        elif expected_response is None:
            expected_response = command

        sock = self._require_socket()
        try:
            sock.sendall(self._encode_command(command, data))
        except OSError as exc:
            self._discard_socket()
            raise WulpusProWiFiDisconnected(str(exc)) from exc

        if expected_response is None:
            return None, None

        deadline = None if timeout is None else time.monotonic() + timeout
        while True:
            remaining = None if deadline is None else deadline - time.monotonic()
            if remaining is not None and remaining <= 0:
                raise WulpusProWiFiTimeout(
                    f"Timed out waiting for {expected_response} acknowledgement"
                )
            header, payload = self.receive_command(remaining)
            if header.command == expected_response:
                return header, payload
            if header.command == WulpusProWiFiCommand.GET_DATA:
                logger.debug(
                    "Discarding asynchronous RF frame while waiting for %s",
                    expected_response,
                )
                continue
            if header.command == WulpusProWiFiCommand.BUSY:
                raise WulpusProWiFiBusy(
                    "WULPUS PRO is busy or controlled through another transport"
                )
            raise WulpusProWiFiProtocolError(
                f"Expected {expected_response}, received {header.command}"
            )

    def flush(self) -> None:
        if self.sock is None:
            self.backlog = b""
            return

        sock = self.sock
        original_timeout = sock.gettimeout()
        sock.settimeout(0.05)
        try:
            while True:
                try:
                    chunk = sock.recv(4096)
                except socket.timeout:
                    break
                if not chunk:
                    self._discard_socket()
                    break
        except OSError as exc:
            self._discard_socket()
            raise WulpusProWiFiDisconnected(str(exc)) from exc
        finally:
            if self.sock is sock:
                sock.settimeout(original_timeout)
        self.backlog = b""

    def send_config(self, conf_bytes_pack: bytes) -> None:
        self.send_command(WulpusProWiFiCommand.SET_CONFIG, conf_bytes_pack)

    @staticmethod
    def _decode_rf_frame(payload: bytes) -> WulpusProFrame:
        if len(payload) < 4:
            raise WulpusProWiFiProtocolError(
                f"RF payload is too short: {len(payload)} bytes"
            )
        if (len(payload) - 4) % 2:
            raise WulpusProWiFiProtocolError("RF sample payload is not 16-bit aligned")
        _, tx_rx_id, acquisition_number = struct.unpack("<BBH", payload[:4])
        samples = np.frombuffer(payload[4:], dtype="<i2")
        return WulpusProFrame(
            samples=samples,
            acquisition_number=acquisition_number,
            tx_rx_id=tx_rx_id,
        )

    def _extract_packet_from_backlog(
        self,
    ) -> Optional[Tuple[WulpusProWiFiHeader, bytes]]:
        while True:
            if len(self.backlog) < HEADER_LENGTH:
                return None
            if not self.backlog.startswith(HEADER_MAGIC):
                next_header = self.backlog.find(HEADER_MAGIC, 1)
                self.backlog = (
                    self.backlog[-(len(HEADER_MAGIC) - 1) :]
                    if next_header < 0
                    else self.backlog[next_header:]
                )
                continue
            try:
                header = self._decode_header(self.backlog[:HEADER_LENGTH])
            except WulpusProWiFiProtocolError:
                self.backlog = self.backlog[1:]
                continue
            packet_length = HEADER_LENGTH + header.length
            if len(self.backlog) < packet_length:
                return None
            payload = self.backlog[HEADER_LENGTH:packet_length]
            self.backlog = self.backlog[packet_length:]
            return header, payload

    def receive_frame(self, timeout: float = 5.0) -> Optional[WulpusProFrame]:
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            try:
                header, payload = self._receive_packet(remaining)
            except WulpusProWiFiTimeout:
                return None
            if header.command != WulpusProWiFiCommand.GET_DATA:
                logger.debug("Ignoring asynchronous %s packet", header.command)
                continue
            frame = self._decode_rf_frame(payload)
            if len(frame.samples) != self.acq_length:
                raise WulpusProWiFiProtocolError(
                    f"Expected {self.acq_length} samples, got {len(frame.samples)}"
                )
            return frame

    def receive_data(
        self, timeout: float = 5.0
    ) -> Optional[Tuple[np.ndarray, int, int]]:
        frame = self.receive_frame(timeout)
        if frame is None:
            return None
        return frame.samples, frame.acquisition_number, frame.tx_rx_id

    def ping(self, timeout: float = 5.0) -> Tuple[WulpusProWiFiHeader, bytes]:
        # command_recv() first echoes the PING header as an acknowledgement;
        # app_main then sends the separate PONG response.
        self.send_command(
            WulpusProWiFiCommand.PING,
            expected_response=WulpusProWiFiCommand.PING,
            timeout=timeout,
        )
        header, payload = self.receive_command(timeout)
        if header.command != WulpusProWiFiCommand.PONG:
            raise WulpusProWiFiProtocolError(
                f"Expected {WulpusProWiFiCommand.PONG}, received {header.command}"
            )
        return header, payload

    def get_status(self, timeout: float = 5.0) -> WulpusProStatus:
        """Return the ESP32 runtime status and sticky acquisition errors."""
        self.send_command(WulpusProWiFiCommand.GET_STATUS, timeout=timeout)
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise WulpusProWiFiTimeout("Timed out waiting for STATUS")
            header, payload = self.receive_command(remaining)
            if header.command == WulpusProWiFiCommand.GET_DATA:
                continue
            if header.command != WulpusProWiFiCommand.STATUS:
                raise WulpusProWiFiProtocolError(
                    f"Expected {WulpusProWiFiCommand.STATUS}, received {header.command}"
                )
            if len(payload) != struct.calcsize("<BBHIIIIIIIIHH"):
                raise WulpusProWiFiProtocolError(
                    f"Invalid STATUS payload length {len(payload)}"
                )
            values = struct.unpack("<BBHIIIIIIIIHH", payload)
            version, size, _, *fields = values
            if size != len(payload):
                raise WulpusProWiFiProtocolError(
                    f"STATUS reports size {size}, received {len(payload)}"
                )
            return WulpusProStatus(version, *fields)

    def clear_status(
        self,
        error_mask: int = 0xFFFFFFFF,
        clear_counters: bool = False,
        timeout: float = 5.0,
    ) -> None:
        """Clear selected sticky errors and optionally diagnostic counters."""
        payload = struct.pack("<IB", error_mask & 0xFFFFFFFF, clear_counters)
        self.send_command(
            WulpusProWiFiCommand.CLEAR_STATUS, payload, timeout=timeout
        )

    def toggle_rx(self, state: bool) -> None:
        command = (
            WulpusProWiFiCommand.START_RX
            if state
            else WulpusProWiFiCommand.STOP_RX
        )
        self.send_command(command)
        self.rx_enabled = state

    def acquire(
        self,
        config: Any,
        frame_timeout: float = 5.0,
        restart_delay: float = 2.5,
    ) -> List[WulpusProFrame]:
        """Configure the MSP430, acquire frames, and return it to a safe state."""
        if not self.connected:
            raise WulpusProWiFiDisconnected("WULPUS PRO Wi-Fi link is not open")

        self.acq_length = int(config.num_samples)
        frames: List[WulpusProFrame] = []
        acquisition_started = False
        try:
            self.toggle_rx(False)
            self.send_config(config.get_restart_package())
            time.sleep(restart_delay)
            self.send_config(config.get_conf_package())
            self.toggle_rx(True)
            acquisition_started = True

            for _ in range(int(config.num_acqs)):
                frame = self.receive_frame(frame_timeout)
                if frame is None:
                    raise WulpusProWiFiTimeout("Timed out waiting for an RF frame")
                frames.append(frame)
            return frames
        finally:
            if self.connected:
                try:
                    if acquisition_started or self.rx_enabled:
                        self.toggle_rx(False)
                    self.send_config(config.get_restart_package())
                except WulpusProWiFiError:
                    logger.exception("Could not return MSP430 to its safe state")

    def reset(self) -> None:
        self.send_command(WulpusProWiFiCommand.RESET)

    def reset_msp(self) -> None:
        """Reset the MSP430 while keeping the current transport session open."""
        self.send_command(WulpusProWiFiCommand.RESET_MSP)

    def _discard_socket(self) -> None:
        sock, self.sock = self.sock, None
        self.device = None
        self.backlog = b""
        self.rx_enabled = False
        if sock is not None:
            try:
                sock.close()
            except OSError:
                pass

    def close(self) -> bool:
        if self.sock is None:
            return True
        try:
            self.send_command(WulpusProWiFiCommand.CLOSE)
        except WulpusProWiFiError:
            logger.warning("Could not send CLOSE command", exc_info=True)
        finally:
            self._discard_socket()
        return True
