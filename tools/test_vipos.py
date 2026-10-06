#!/usr/bin/env python3
from pathlib import Path
import hashlib
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
boot = (ROOT / "build/boot.bin").read_bytes()
kernel = (ROOT / "build/kernel.bin").read_bytes()
vip = (ROOT / "build/demo.vip").read_bytes()
img = (ROOT / "vipos.img").read_bytes()

SECTOR = 512
KERNEL_SECTORS = 32
VIP_SECTORS = 8

assert len(boot) == SECTOR
assert boot[510:512] == b"\x55\xaa"
assert len(kernel) == KERNEL_SECTORS * SECTOR
assert len(vip) == VIP_SECTORS * SECTOR
assert len(img) == (1 + KERNEL_SECTORS + VIP_SECTORS) * SECTOR
assert img[:SECTOR] == boot
assert img[SECTOR:SECTOR + KERNEL_SECTORS * SECTOR] == kernel
assert img[(1 + KERNEL_SECTORS) * SECTOR:] == vip

fmt = "<4sHHIIII32s"
header_size = struct.calcsize(fmt)
magic, version, declared_header, manifest_size, payload_size, payload_type, _reserved, digest = struct.unpack_from(fmt, vip)
assert magic == b"VIP0"
assert version == 1
assert declared_header == header_size
assert payload_type == 1

manifest_start = declared_header
manifest_end = manifest_start + manifest_size
payload_end = manifest_end + payload_size
assert payload_end <= len(vip)

manifest = vip[manifest_start:manifest_end]
payload = vip[manifest_end:payload_end]
assert hashlib.sha256(payload).digest() == digest
assert b"name=VIPOS Hello" in manifest
assert b"id=vip.demo.hello" in manifest
assert b"version=1.0" in manifest
assert payload[-1] == 0xFF

result = subprocess.run(
    ["readelf", "-h", str(ROOT / "build/kernel.elf")],
    capture_output=True,
    text=True,
    check=True,
)
out = result.stdout
assert "ELF32" in out
assert "Intel 80386" in out
assert "0x8000" in out

print("All VIPOS automated build/package checks passed.")
