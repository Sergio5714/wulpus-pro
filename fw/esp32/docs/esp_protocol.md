# ESP32-to-PC protocol

The ESP32 exposes the same ordered byte-stream protocol over Wi-Fi/TCP and the
ESP32-C6 native USB Serial/JTAG CDC interface. Packets, command IDs, payloads,
and acknowledgements are transport-independent.

The [WULPUS PRO WiFi host PCB](../../../hw/wulpus_wifi_host_pcb), containing a
XIAO ESP32-C6, is the primary host board for this protocol. Standalone XIAO and
ESP32-C6-DEVKITM-1 boards use the same protocol for development.

## Transports

| Transport | Endpoint | Notes |
|---|---|---|
| Wi-Fi TCP | TCP port 2121 by default | Discovered through the `_wulpus._tcp` mDNS service after Wi-Fi obtains an IP address. |
| USB CDC | ESP32-C6 USB Serial/JTAG COM port | Baud-rate settings do not control USB line speed. The port must not be open in another GUI, terminal, or monitor. |

Only one transport owns a protocol session at a time. See
[Firmware architecture](architecture.md#usb-and-wi-fi-session-switching) for
arbitration and switching.

## Packet format

Every message is one packed nine-byte header followed immediately by
`data_length` payload bytes:

| Offset | Size | Field | Encoding |
|---:|---:|---|---|
| 0 | 6 | `magic` | ASCII `wulpus` (`77 75 6c 70 75 73`). |
| 6 | 1 | `command` | Command ID from the table below. |
| 7 | 2 | `data_length` | Unsigned little-endian payload length, 0–65535. |
| 9 | variable | `payload` | Command-specific bytes. |

In Python, a packet can be encoded as:

```python
packet = b"wulpus" + struct.pack("<BH", command, len(payload)) + payload
```

TCP and USB may split or combine packets arbitrarily. A receiver must accumulate
bytes until the complete header and declared payload are available; one socket
read or serial read is not guaranteed to equal one protocol packet.

Before a session is claimed, each listener scans the stream for the `wulpus`
magic. This also recovers from boot text or stale serial bytes. During an active
session, a malformed header or oversized command payload is a protocol error
and ends the session rather than silently resynchronizing in the middle of a
packet.

## Commands

| ID | Name | Direction | Request payload | Responses and effect |
|---:|---|---|---|---|
| `0x57` | `SET_CONFIG` | PC -> ESP | MSP430 configuration package, at most 804 bytes | Empty `SET_CONFIG` acknowledgement is sent before the ESP waits for DATA_READY and transfers the zero-padded 804-byte block to MSP430. |
| `0x58` | `GET_DATA` | ESP -> PC | 804-byte RF payload | Asynchronous acquisition frame emitted while RX is enabled. It is not a host polling request. |
| `0x59` | `PING` | PC -> ESP | Empty | Empty `PING` acknowledgement, followed by `PONG` containing ASCII `pong`. |
| `0x5A` | `PONG` | ESP -> PC | ASCII `pong` | Second response to `PING`. |
| `0x5B` | `RESET` | PC -> ESP | Empty | Empty `RESET` acknowledgement is queued first; acquisition is stopped, MSP430 is shut down/reset, and the ESP32 restarts. |
| `0x5C` | `CLOSE` | PC -> ESP | Empty | Acquisition is stopped, then an empty `CLOSE` acknowledgement is sent. The session proceeds through common cleanup and releases ownership. |
| `0x5D` | `START_RX` | PC -> ESP | Empty | Empty `START_RX` acknowledgement is sent before acquisition forwarding is enabled. |
| `0x5E` | `STOP_RX` | PC -> ESP | Empty | Acquisition and queued session frames are stopped/discarded before the empty `STOP_RX` acknowledgement is sent. The protocol session remains open. |
| `0x5F` | `BUSY` | ESP -> PC | Empty | Returned when another transport already owns the session. |
| `0x60` | `GET_STATUS` | PC -> ESP | Empty | Empty `GET_STATUS` acknowledgement followed by one `STATUS` packet. |
| `0x61` | `STATUS` | ESP -> PC | 40-byte versioned status payload | Runtime errors, counters, and frame-pool occupancy. |
| `0x62` | `CLEAR_STATUS` | PC -> ESP | Empty or five-byte clear request | Requested flags/counters are cleared before the empty `CLEAR_STATUS` acknowledgement is sent. |

Command values outside `0x57`–`0x62` are invalid in the current protocol.
Commands documented as ESP-to-PC should not be sent by a host.

## Acknowledgement model

Most PC commands receive an empty packet with the same command ID as their
acknowledgement. The acknowledgement indicates that the command was accepted by
the protocol task; for commands acknowledged before their action, it does not
prove that a later SPI operation succeeded. Use `GET_STATUS` for asynchronous
SPI and buffer errors.

Acknowledgements are ordered with acquisition data by the sole packet-TX
thread. Because RF frames are asynchronous, a `GET_DATA` packet may already be
buffered when the host begins waiting for a command response. The host parser
must consume complete packets and continue until it finds the expected command
ID. It must never interpret arbitrary bytes following an RF payload as a
response header.

Special response sequences are:

```text
PING        -> PING acknowledgement -> PONG("pong")
GET_STATUS  -> GET_STATUS acknowledgement -> STATUS(payload)
CLEAR_STATUS -> CLEAR_STATUS acknowledgement after clearing
CLOSE       -> CLOSE acknowledgement after stopping RX
```

## Acquisition frame

`GET_DATA` has an 804-byte payload:

| Offset | Size | Field | Encoding |
|---:|---:|---|---|
| 0 | 1 | `frame_marker` | `0xFF` from MSP430. |
| 1 | 1 | `tx_rx_id` | Active TX/RX configuration index. |
| 2 | 2 | `acquisition_number` | Unsigned little-endian 16-bit sequence number. |
| 4 | 800 | `samples` | 400 little-endian signed 16-bit samples. |

Including the outer header, one acquisition packet occupies 813 application
bytes. At 500 FPS this requires 406,500 bytes/s (3.252 Mbit/s) before USB or TCP
transport overhead.

The acquisition number is a wrapping sequence value, not an array index and not
a reliable indication of progress within a newly requested GUI run. Detect a
gap with modulo-65536 arithmetic:

```python
gap = ((current - previous) & 0xFFFF) != 1
```

## Runtime status

The version-1 `STATUS` payload is a packed 40-byte little-endian structure:

| Offset | Size | Field | Meaning |
|---:|---:|---|---|
| 0 | 1 | `version` | Status schema version; currently 1. |
| 1 | 1 | `size` | Total payload size; currently 40. |
| 2 | 2 | `reserved` | Reserved, currently zero. |
| 4 | 4 | `error_flags` | Sticky error bit mask. |
| 8 | 4 | `buffer_overflow_count` | Times acquisition could not obtain a free frame slot. |
| 12 | 4 | `data_ready_count` | DATA_READY edges serviced by the acquisition thread. |
| 16 | 4 | `completed_spi_count` | Successful acquisition SPI transactions. |
| 20 | 4 | `transmitted_frame_count` | RF frames successfully written to the active link. |
| 24 | 4 | `discarded_frame_count` | Completed frames released without transmission, normally during stop/session cleanup or generation changes. |
| 28 | 4 | `spi_error_count` | Failed or timed-out SPI transfers. |
| 32 | 4 | `link_error_count` | Packet-write/link failures. |
| 36 | 2 | `current_buffer_usage` | Frame slots currently owned by SPI, READY, or TX. |
| 38 | 2 | `maximum_buffer_usage` | High-water mark since initialization or counter reset. |

### Error flags

| Bit | Mask | Name | Meaning |
|---:|---:|---|---|
| 0 | `0x00000001` | `ACQ_BUFFER_OVERFLOW` | No free DMA frame slot became available within the acquisition wait. Acquisition is stopped. |
| 1 | `0x00000002` | `DATA_READY_OVERFLOW` | Reserved by the current status schema for lost/excess DATA_READY events. |
| 2 | `0x00000004` | `SPI_TIMEOUT` | MSP430 edge or SPI shutdown/transfer timing failed. |
| 3 | `0x00000008` | `SPI_FAILURE` | Non-timeout SPI operation failed. |
| 4 | `0x00000010` | `LINK_TIMEOUT` | A packet could not be written within the link timeout. |
| 5 | `0x00000020` | `LINK_DISCONNECTED` | Reserved for explicit link-disconnect reporting. |
| 6 | `0x00000040` | `PROTOCOL` | Invalid active-session header or payload length. |

Error flags are sticky until `CLEAR_STATUS` or reboot. Counters are unsigned
32-bit values and are not automatically reset at the beginning of a host
acquisition unless the host requests it.

## Clearing status

An empty `CLEAR_STATUS` request clears all error bits but preserves counters.
The optional five-byte request payload is:

| Offset | Size | Field | Meaning |
|---:|---:|---|---|
| 0 | 4 | `error_mask` | Little-endian bit mask of sticky errors to clear. |
| 4 | 1 | `clear_counters` | Nonzero clears all diagnostic counters and resets the frame-pool high-water mark. |

Example Python payload:

```python
payload = struct.pack("<IB", 0xFFFFFFFF, 1)
```

## Typical session

```text
PC                                      ESP32 / MSP430
--                                      --------------
connect/open CDC
PING ---------------------------------> claim session, release MSP430 reset
     <------------------------------- PING acknowledgement
     <------------------------------- PONG "pong"
SET_CONFIG(package) ------------------> transfer configuration to MSP430
     <------------------------------- SET_CONFIG acknowledgement
CLEAR_STATUS(mask, counters=1) -------> clear diagnostics
     <------------------------------- CLEAR_STATUS acknowledgement
START_RX -----------------------------> enable forwarding
     <------------------------------- START_RX acknowledgement
     <------------------------------- GET_DATA frame 0
     <------------------------------- GET_DATA frame 1
     <------------------------------- ...
STOP_RX ------------------------------> stop and discard queued session frames
     <------------------------------- STOP_RX acknowledgement
GET_STATUS ---------------------------> snapshot diagnostics
     <------------------------------- GET_STATUS acknowledgement
     <------------------------------- STATUS payload
CLOSE --------------------------------> stop and clean up MSP430/session
     <------------------------------- CLOSE acknowledgement
```

The Python implementations are `WulpusProWiFiLink` and
`WulpusProUsbCdcLink` under `sw/wulpus/`. They share one framed parser so TCP
and USB follow identical packet-boundary and acknowledgement rules.
