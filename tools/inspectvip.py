#!/usr/bin/env python3
import pathlib
import struct
import sys

HEADER_FMT = "<4sHHIIII32s"
HEADER_SIZE = struct.calcsize(HEADER_FMT)

p = pathlib.Path(sys.argv[1])
b = p.read_bytes()
if len(b) < HEADER_SIZE:
    raise SystemExit("invalid: file too small")
magic, version, header_size, manifest_size, payload_size, payload_type, reserved, digest = struct.unpack_from(HEADER_FMT, b)
if magic != b"VIP0":
    raise SystemExit("invalid: bad magic")
end_m = header_size + manifest_size
end_p = end_m + payload_size
if end_p > len(b):
    raise SystemExit("invalid: truncated package")
manifest = b[header_size:end_m].decode("utf-8")
print(f"magic={magic.decode()}")
print(f"version={version}")
print(f"header_size={header_size}")
print(f"manifest_size={manifest_size}")
print(f"payload_size={payload_size}")
print(f"payload_type={payload_type}")
print("manifest:")
print(manifest, end="")
print(f"sha256={digest.hex()}")
