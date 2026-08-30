"""Profile WULPUS PRO frame rate over ESP32-C6 native USB CDC."""

import argparse
import statistics
import sys
import time
from typing import List

from wulpus.usb_cdc_link import WulpusProUsbCdcLink
from wulpus.uss_conf_pro import WulpusProUssConfig
from wulpus.wifi_link import WulpusProWiFiError

DCDC_AFTER_PERIOD_MARGIN_US = 1000
MAX_SLOW_TIMER_PERIOD_US = 2_000_000


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Measure sustained USB CDC frame rate. TX is disabled by default; "
            "close the GUI and serial monitors before running."
        )
    )
    parser.add_argument("--port", default="COM9", help="USB CDC port (default: COM9)")
    parser.add_argument(
        "--period-us",
        type=int,
        nargs="+",
        required=True,
        help="Explicit acquisition periods to test, in microseconds",
    )
    parser.add_argument("--frames", type=int, default=500, help="Measured frames per period")
    parser.add_argument("--warmup", type=int, default=10, help="Frames discarded before timing")
    parser.add_argument("--samples", type=int, default=400, help="16-bit samples per frame")
    parser.add_argument("--timeout", type=float, default=5.0, help="Per-frame timeout in seconds")
    parser.add_argument(
        "--tx-mask",
        type=lambda value: int(value, 0),
        default=0,
        help="TX channel bit mask; default 0 disables pulsing",
    )
    parser.add_argument(
        "--rx-mask",
        type=lambda value: int(value, 0),
        default=0,
        help="RX channel bit mask (default: 0)",
    )
    return parser.parse_args()


def sequence_gap_count(numbers: List[int]) -> int:
    return sum(
        ((current - previous) & 0xFFFF) != 1
        for previous, current in zip(numbers, numbers[1:])
    )


def profile_period(link: WulpusProUsbCdcLink, args: argparse.Namespace, period_us: int) -> bool:
    dcdc_turnon_us = period_us + DCDC_AFTER_PERIOD_MARGIN_US
    config = WulpusProUssConfig(
        num_acqs=args.frames + args.warmup,
        dcdc_turnon=dcdc_turnon_us,
        meas_period=period_us,
        num_samples=args.samples,
        num_txrx_configs=1,
        tx_configs=[args.tx_mask],
        rx_configs=[args.rx_mask],
    )
    link.acq_length = args.samples
    timestamps: List[float] = []
    acquisition_numbers: List[int] = []
    timed_out = False
    total_received = 0

    link.toggle_rx(False)
    link.send_config(config.get_restart_package())
    time.sleep(2.5)
    link.send_config(config.get_conf_package())
    link.toggle_rx(True)
    try:
        for index in range(args.frames + args.warmup):
            frame = link.receive_frame(args.timeout)
            if frame is None:
                timed_out = True
                break
            total_received += 1
            if index >= args.warmup:
                timestamps.append(time.perf_counter())
                acquisition_numbers.append(frame.acquisition_number)
    finally:
        link.toggle_rx(False)
        link.send_config(config.get_restart_package())

    target_fps = 1_000_000 / period_us
    if len(timestamps) < 2:
        print(
            f"{period_us:9d} us  target={target_fps:9.2f} FPS  "
            f"insufficient frames (received={total_received}, "
            f"warmup={min(total_received, args.warmup)}, "
            f"measured={len(timestamps)}, timeout={timed_out})"
        )
        return False

    intervals = [later - earlier for earlier, later in zip(timestamps, timestamps[1:])]
    average_fps = (len(timestamps) - 1) / (timestamps[-1] - timestamps[0])
    median_fps = 1 / statistics.median(intervals)
    p95_interval = sorted(intervals)[int(0.95 * (len(intervals) - 1))]
    gaps = sequence_gap_count(acquisition_numbers)
    stable = not timed_out and gaps == 0 and len(timestamps) == args.frames
    status = "PASS" if stable else "FAIL"
    print(
        f"{period_us:9d} us  target={target_fps:9.2f} FPS  "
        f"average={average_fps:9.2f}  median={median_fps:9.2f}  "
        f"p95_latency={p95_interval * 1000:8.3f} ms  "
        f"frames={len(timestamps):5d}  gaps={gaps:3d}  {status}"
    )
    return stable


def main() -> int:
    args = parse_args()
    if args.frames < 2 or args.warmup < 0 or args.samples <= 0:
        raise SystemExit("frames must be >= 2, warmup >= 0, and samples > 0")
    if any(period <= 0 for period in args.period_us):
        raise SystemExit("all acquisition periods must be greater than zero")
    if any(
        period + DCDC_AFTER_PERIOD_MARGIN_US > MAX_SLOW_TIMER_PERIOD_US
        for period in args.period_us
    ):
        raise SystemExit(
            "each acquisition period must leave room for dcdc_turnon to be "
            f"{DCDC_AFTER_PERIOD_MARGIN_US} us later (maximum period: "
            f"{MAX_SLOW_TIMER_PERIOD_US - DCDC_AFTER_PERIOD_MARGIN_US} us)"
        )

    link = WulpusProUsbCdcLink(port=args.port, timeout=args.timeout)
    if not link.open():
        print(f"Could not open {args.port}; close the GUI and serial monitors", file=sys.stderr)
        return 2

    results: List[bool] = []
    try:
        pong = link.ping(args.timeout)
        print(f"Connected to {args.port}; handshake payload={pong[1]!r}")
        print(
            "Safety: TX mask is "
            f"0x{args.tx_mask:04X}; dcdc_turnon is always acquisition period + "
            f"{DCDC_AFTER_PERIOD_MARGIN_US} us"
        )
        print(
            "   period             requested             measured"
            "                         integrity"
        )
        for period_us in args.period_us:
            results.append(profile_period(link, args, period_us))
    except WulpusProWiFiError as exc:
        print(f"Profiling failed: {exc}", file=sys.stderr)
        return 3
    finally:
        link.close()

    passing = [period for period, passed in zip(args.period_us, results) if passed]
    if passing:
        fastest = min(passing)
        print(f"Fastest stable tested period: {fastest} us ({1_000_000 / fastest:.2f} FPS target)")
    else:
        print("No tested period completed without timeout or sequence gaps")
    return 0 if all(results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
