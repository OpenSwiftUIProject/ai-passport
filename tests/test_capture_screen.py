import base64
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from capture_screen import CaptureError, Frame, save_png


def row(x, y, raw, request_id="12345678"):
    return (f"FPS1 ROW {request_id} {x} {y} {len(raw)//2} {zlib.crc32(raw):08x} "
            + base64.b64encode(raw).decode()).encode()


class ScreenCaptureTests(unittest.TestCase):
    def test_firmware_encoder_and_png_colors(self):
        wire = subprocess.check_output([sys.argv[1]])
        frame = Frame("12345678", (3, 1))
        for line in wire.splitlines():
            frame.accept(line)
        png = frame.png()
        self.assertEqual(png[:8], b"\x89PNG\r\n\x1a\n")
        offset, compressed = 8, b""
        while offset < len(png):
            length = struct.unpack_from(">I", png, offset)[0]
            kind, payload = png[offset+4:offset+8], png[offset+8:offset+8+length]
            self.assertEqual(struct.unpack_from(">I", png, offset+8+length)[0], zlib.crc32(kind+payload))
            if kind == b"IDAT":
                compressed += payload
            offset += length+12
        self.assertEqual(zlib.decompress(compressed), b"\0\xff\0\0\0\xff\0\0\0\xff")

    def test_tiled_rows_and_unrelated_serial_logs(self):
        frame = Frame("12345678", (2, 2))
        self.assertFalse(frame.accept(b"device diagnostic"))
        self.assertFalse(frame.accept(b"FPS1 ERROR deadbeef stale request"))
        frame.accept(b"FPS1 BEGIN 12345678 2 2 RGB565LE")
        for x, y, pixel in [(1, 1, b"\xff\xff"), (0, 0, b"\0\xf8"), (0, 1, b"\x1f\0"), (1, 0, b"\xe0\x07")]:
            frame.accept(row(x, y, pixel))
        self.assertTrue(frame.accept(b"FPS1 END 12345678 4"))
        self.assertEqual(frame.pixels, b"\0\xf8\xe0\x07\x1f\0\xff\xff")

    def test_rejects_corruption_missing_data_and_bad_coordinates(self):
        records = [row(0, 0, b"\0\xf8").replace(b"APg=", b"AAAA"),
                   row(0, 0, b"\0\xf8").replace(b"APg=", b"APk="),
                   b"FPS1 END 12345678 2", row(2, 0, b"\0\xf8"),
                   row(0, -1, b"\0\xf8"), b"FPS1 ROW 12345678 0 0 1 00000000 !!!!"]
        for record in records:
            with self.subTest(record=record):
                frame = Frame("12345678", (2, 1))
                frame.accept(b"FPS1 BEGIN 12345678 2 1 RGB565LE")
                with self.assertRaises(CaptureError):
                    frame.accept(record)

    def test_rejects_duplicates_and_device_errors(self):
        frame = Frame("12345678", (1, 1))
        frame.accept(b"FPS1 BEGIN 12345678 1 1 RGB565LE")
        packet = row(0, 0, b"\0\xf8")
        frame.accept(packet)
        with self.assertRaises(CaptureError):
            frame.accept(packet)
        with self.assertRaisesRegex(CaptureError, "ESP_ERR_TIMEOUT"):
            frame.accept(b"FPS1 ERROR 12345678 ESP_ERR_TIMEOUT")

    def test_incomplete_frame_does_not_replace_existing_file(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "screen.png"
            output.write_bytes(b"previous capture")
            with self.assertRaises(CaptureError):
                save_png(Frame("12345678", (1, 1)), output)
            self.assertEqual(output.read_bytes(), b"previous capture")


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
