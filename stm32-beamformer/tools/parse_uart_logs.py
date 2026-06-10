#!/usr/bin/env python3
"""
Parse and analyze UART output from STM32 beamformer.

Extracts beamformer metrics, plots results, and provides statistics.

Usage:
    python3 parse_uart_logs.py /dev/ttyUSB0
    python3 parse_uart_logs.py logfile.txt --plot --stats
"""

import serial
import argparse
import sys
from collections import deque
from datetime import datetime


class BeamformerParser:
    """Parse STM32 beamformer UART output."""

    def __init__(self, max_samples=1000):
        self.max_samples = max_samples
        self.frames = deque(maxlen=max_samples)
        self.powers = deque(maxlen=max_samples)
        self.qualities = deque(maxlen=max_samples)
        self.timestamps = deque(maxlen=max_samples)

    def parse_line(self, line):
        """Parse a single UART line."""
        line = line.strip()

        if not line:
            return False

        try:
            # Example format: "Frame: 100, Power: 45 dB, Quality: 95%"
            if "Frame:" in line and "Power:" in line:
                parts = line.split(',')
                frame_num = int(parts[0].split(':')[1].strip())
                power_db = int(parts[1].split(':')[1].strip().split()[0])
                quality = int(parts[2].split(':')[1].strip().rstrip('%'))

                self.frames.append(frame_num)
                self.powers.append(power_db)
                self.qualities.append(quality)
                self.timestamps.append(datetime.now())
                return True

        except (ValueError, IndexError) as e:
            print(f"Parse error: {e} in line: {line}", file=sys.stderr)

        return False

    def get_stats(self):
        """Calculate statistics."""
        if not self.powers:
            return None

        powers = list(self.powers)
        qualities = list(self.qualities)

        stats = {
            'num_frames': len(self.frames),
            'avg_power_db': sum(powers) / len(powers),
            'min_power_db': min(powers),
            'max_power_db': max(powers),
            'avg_quality': sum(qualities) / len(qualities),
            'min_quality': min(qualities),
            'max_quality': max(qualities),
        }

        # Calculate frame rate
        if len(self.timestamps) > 1:
            time_delta = (self.timestamps[-1] - self.timestamps[0]).total_seconds()
            if time_delta > 0:
                stats['frame_rate_hz'] = len(self.frames) / time_delta

        return stats

    def print_stats(self):
        """Print statistics to console."""
        stats = self.get_stats()
        if not stats:
            print("No data collected")
            return

        print("\n=== Beamformer Statistics ===")
        print(f"Frames processed: {stats['num_frames']}")
        print(f"Average power: {stats['avg_power_db']:.1f} dB")
        print(f"Power range: {stats['min_power_db']} to {stats['max_power_db']} dB")
        print(f"Average quality: {stats['avg_quality']:.1f}%")
        print(f"Quality range: {stats['min_quality']} to {stats['max_quality']}%")
        if 'frame_rate_hz' in stats:
            print(f"Frame rate: {stats['frame_rate_hz']:.1f} Hz")


def read_serial(port, baudrate=115200, timeout=60):
    """Read from serial port."""
    parser = BeamformerParser()

    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        print(f"Connected to {port} at {baudrate} baud")
        print("Listening... (Ctrl+C to stop)\n")

        while True:
            line = ser.readline().decode('utf-8', errors='replace')
            if line:
                print(line.strip())
                parser.parse_line(line)

    except KeyboardInterrupt:
        print("\n\nStopped by user")
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        sys.exit(1)
    finally:
        if ser.is_open:
            ser.close()

    return parser


def read_file(filename):
    """Read from log file."""
    parser = BeamformerParser()

    try:
        with open(filename, 'r') as f:
            for line in f:
                parser.parse_line(line)
                print(line.strip())

    except FileNotFoundError:
        print(f"File not found: {filename}")
        sys.exit(1)

    return parser


def plot_data(parser):
    """Plot beamformer data (requires matplotlib)."""
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not installed. Install with: pip install matplotlib")
        return

    if not parser.powers:
        print("No data to plot")
        return

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

    # Plot power
    ax1.plot(list(parser.powers), 'b-', linewidth=0.5)
    ax1.set_ylabel('Power (dB)', fontsize=12)
    ax1.set_title('Beamformer Power vs. Frame', fontsize=14)
    ax1.grid(True, alpha=0.3)

    # Plot quality
    ax2.plot(list(parser.qualities), 'g-', linewidth=0.5)
    ax2.set_ylabel('Quality (%)', fontsize=12)
    ax2.set_xlabel('Frame Number', fontsize=12)
    ax2.set_title('Beam Quality vs. Frame', fontsize=14)
    ax2.set_ylim([0, 105])
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.show()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Parse STM32 beamformer UART output')
    parser.add_argument('input', nargs='?', help='Serial port or log file')
    parser.add_argument('--plot', action='store_true', help='Plot data')
    parser.add_argument('--stats', action='store_true', help='Print statistics')
    parser.add_argument('--baud', type=int, default=115200, help='Serial baudrate')
    args = parser.parse_args()

    if not args.input:
        parser.print_help()
        sys.exit(1)

    # Determine if input is serial port or file
    if args.input.startswith('/dev/') or args.input.startswith('COM'):
        # Serial port
        data_parser = read_serial(args.input, baudrate=args.baud)
    else:
        # File
        data_parser = read_file(args.input)

    # Print stats
    if args.stats:
        data_parser.print_stats()

    # Plot if requested
    if args.plot:
        plot_data(data_parser)
