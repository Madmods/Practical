"""Read-only live meter/key capture for comparing release timing.

Usage: capture_overhold.py PID MODULE_BASE OUTPUT.csv [SECONDS]
Does not send input or write process memory.
"""
import csv
import ctypes
import runpy
import struct
import sys
import time
from pathlib import Path

pid, base, output = sys.argv[1:4]
duration = float(sys.argv[4]) if len(sys.argv) > 4 else 18
sys.argv = ["meter_objects.py", pid, base]
reader = runpy.run_path(str(Path(__file__).resolve().parents[4] / "Tools" / "Diagnostics" / "src" / "meter_objects.py"))
if not reader["meter"]:
    raise SystemExit("No live meter found; capture not started.")
user32 = ctypes.WinDLL("user32")
user32.GetAsyncKeyState.argtypes = [ctypes.c_int]
user32.GetAsyncKeyState.restype = ctypes.c_short
rows = []
start = time.perf_counter()
print(f"Capturing for {duration:g}s", flush=True)
while time.perf_counter() - start < duration:
    elapsed = time.perf_counter() - start
    fill = -struct.unpack("<f", reader["raw"](reader["meter"] + 0x12c, 4))[0]
    rows.append((round(elapsed, 5), round(fill, 6), int(bool(user32.GetAsyncKeyState(69) & 0x8000))))
    time.sleep(.002)
with Path(output).open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("time", "fill", "e_down"))
    writer.writerows(rows)
print(f"Saved {len(rows)} samples to {output}", flush=True)
