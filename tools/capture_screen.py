#!/usr/bin/env python3
"""Capture one checked RGB565 frame from AI Passport over USB, then save PNG.

Requires pyserial for device access (already included in tools/with-env.sh).
Protocol parsing and PNG encoding use only the Python standard library.
"""
from __future__ import annotations

import argparse
import base64
import binascii
import json
import os
from pathlib import Path
import secrets
import struct
import sys
import tempfile
import time
import zlib


class CaptureError(ValueError):
    pass


class Frame:
    """Reject stale, corrupt, overlapping, or incomplete frames before saving."""

    def __init__(self, request_id: str, expected_size: tuple[int, int] = (240, 320)):
        self.request_id = request_id
        self.width, self.height = expected_size
        self.pixels = bytearray(self.width * self.height * 2)
        self.coverage = bytearray(self.width * self.height)
        self.received = 0
        self.started = False
        self.complete = False

    def accept(self, line: bytes) -> bool:
        if not line.startswith(b"FPS1 "):
            return False  # ordinary serial diagnostics
        try:
            fields = line.decode("ascii").split()
            if len(fields) < 3 or fields[2].lower() != self.request_id:
                return False  # a previous request's buffered data
            kind = fields[1]
            if kind == "ERROR":
                raise CaptureError("Device rejected screenshot: " + " ".join(fields[3:]))
            if self.complete:
                raise CaptureError("Unexpected data after END")
            if kind == "BEGIN":
                if self.started or len(fields) != 6:
                    raise CaptureError("Invalid or repeated BEGIN")
                if (int(fields[3]), int(fields[4])) != (self.width, self.height) or fields[5] != "RGB565LE":
                    raise CaptureError("Unexpected dimensions or pixel format")
                self.started = True
            elif kind == "ROW":
                if not self.started or len(fields) != 8:
                    raise CaptureError("ROW without a valid BEGIN")
                x, y, width = map(int, fields[3:6])
                if not (0 <= x < self.width and 0 <= y < self.height and 0 < width <= self.width - x):
                    raise CaptureError("Row coordinates outside the frame")
                raw = base64.b64decode(fields[7], validate=True)
                if len(raw) != width * 2:
                    raise CaptureError("Incorrect row byte count")
                if len(fields[6]) != 8 or zlib.crc32(raw) != int(fields[6], 16):
                    raise CaptureError("Row CRC32 mismatch")
                start = y * self.width + x
                if any(self.coverage[start:start + width]):
                    raise CaptureError("Overlapping or repeated pixel data")
                self.pixels[start * 2:(start + width) * 2] = raw
                self.coverage[start:start + width] = b"\x01" * width
                self.received += width
            elif kind == "END":
                if not self.started or len(fields) != 4:
                    raise CaptureError("Invalid END")
                expected = self.width * self.height
                if int(fields[3]) != expected or self.received != expected:
                    raise CaptureError(f"Incomplete frame: {self.received}/{expected} pixels")
                self.complete = True
            else:
                raise CaptureError("Unknown screenshot record")
        except (UnicodeError, ValueError, binascii.Error) as error:
            if isinstance(error, CaptureError):
                raise
            raise CaptureError("Malformed screenshot record") from error
        return self.complete

    def png(self) -> bytes:
        if not self.complete:
            raise CaptureError("Cannot encode an incomplete frame")
        scanlines = bytearray()
        for y in range(self.height):
            scanlines.append(0)  # PNG filter: None
            for x in range(self.width):
                offset = (y * self.width + x) * 2
                pixel = self.pixels[offset] | (self.pixels[offset + 1] << 8)
                r, g, b = (pixel >> 11) & 31, (pixel >> 5) & 63, pixel & 31
                scanlines.extend(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)))

        def chunk(kind: bytes, data: bytes) -> bytes:
            return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

        return (b"\x89PNG\r\n\x1a\n"
                + chunk(b"IHDR", struct.pack(">IIBBBBB", self.width, self.height, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(scanlines)) + chunk(b"IEND", b""))


def open_serial_without_reset(port) -> None:
    # Match ESP-IDF Monitor's no-reset sequence. Starting with both lines
    # deasserted lets the macOS driver's open-time RTS transition reset C3.
    # Assert both before open, then release RTS first and DTR second.
    port.rts = True
    port.dtr = True
    port.open()
    port.rts = False
    port.dtr = False


def capture(port_name: str | None, timeout: float) -> Frame:
    import serial
    from serial.tools import list_ports

    if port_name is None:
        candidates = [p.device for p in list_ports.comports() if (p.vid, p.pid) == (0x303A, 0x1001)]
        if len(candidates) != 1:
            raise CaptureError("Expected one ESP USB device; use --port to select the connected Passport")
        port_name = candidates[0]
    request_id = secrets.token_hex(4)
    frame = Frame(request_id)
    options = {"exclusive": True} if os.name == "posix" else {}
    port = serial.Serial(port=None, baudrate=115200, timeout=0.2, write_timeout=2, **options)
    port.port = port_name
    try:
        open_serial_without_reset(port)
        port.reset_input_buffer()
        port.write(f"\nFPS1 CAPTURE {request_id}\n".encode("ascii"))
        deadline = time.monotonic() + timeout
        pending = b""
        while time.monotonic() < deadline:
            pending += port.read(4096)
            while b"\n" in pending:
                line, pending = pending.split(b"\n", 1)
                if len(line) > 4096:
                    raise CaptureError("Oversized serial record")
                if frame.accept(line):
                    return frame
            if len(pending) > 4096:
                raise CaptureError("Unterminated serial record")
        raise CaptureError(f"Screenshot timed out ({frame.received}/{frame.width * frame.height} pixels). "
                           "Check that screenshot-enabled firmware is running and close other serial monitors.")
    finally:
        port.close()


def save_png(frame: Frame, output: Path) -> None:
    data = frame.png()  # validate before creating/replacing any output
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary: str | None = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix=".passport-", suffix=".png", delete=False) as file:
            temporary = file.name
            file.write(data)
        os.replace(temporary, output)
    finally:
        if temporary and os.path.exists(temporary):
            os.unlink(temporary)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="USB serial port; autodetects a single Espressif USB device when omitted")
    parser.add_argument("--output", type=Path, required=True, help="Destination PNG (replaced only after successful capture)")
    parser.add_argument("--timeout", type=float, default=15, help="Capture timeout in seconds (default: 15)")
    args = parser.parse_args()
    if not 0 < args.timeout <= 120:
        parser.error("--timeout must be greater than 0 and at most 120")
    try:
        start = time.monotonic()
        frame = capture(args.port, args.timeout)
        output = args.output.expanduser().resolve()
        save_png(frame, output)
        print(json.dumps({"output": str(output), "width": frame.width, "height": frame.height,
                          "pixels_verified": frame.received, "elapsed_seconds": round(time.monotonic() - start, 2)}))
        return 0
    except ImportError:
        print("ERROR: pyserial is required; run with tools/with-env.sh python tools/capture_screen.py ...", file=sys.stderr)
    except (CaptureError, OSError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
