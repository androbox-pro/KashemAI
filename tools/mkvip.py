#!/usr/bin/env python3
import argparse
import hashlib
import pathlib
import struct

MAGIC = b"VIP0"
VERSION = 1
HEADER_FMT = "<4sHHIIII32s"
HEADER_SIZE = struct.calcsize(HEADER_FMT)


def read_manifest(path: pathlib.Path) -> bytes:
    data = path.read_bytes()
    if not data:
        raise ValueError("manifest is empty")
    return data.replace(b"\r\n", b"\n")


def main() -> None:
    ap = argparse.ArgumentParser(description="Build a VIPOS .vip package")
    ap.add_argument("manifest", type=pathlib.Path)
    ap.add_argument("payload", type=pathlib.Path)
    ap.add_argument("output", type=pathlib.Path)
    ap.add_argument("--type", type=int, default=1, dest="payload_type")
    args = ap.parse_args()

    manifest = read_manifest(args.manifest)
    payload = args.payload.read_bytes()
    if args.payload_type < 1:
        raise ValueError("payload type must be >= 1")
    digest = hashlib.sha256(payload).digest()
    header = struct.pack(
        HEADER_FMT,
        MAGIC,
        VERSION,
        HEADER_SIZE,
        len(manifest),
        len(payload),
        args.payload_type,
        0,
        digest,
    )
    blob = header + manifest + payload
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(blob)
    print(f"built {args.output} ({len(blob)} bytes)")


if __name__ == "__main__":
    main()
