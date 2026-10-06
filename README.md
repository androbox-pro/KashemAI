# VIPOS

VIPOS is an educational operating-system prototype focused on building a mobile-oriented OS architecture from the ground up.

> **Current status:** VIPOS 0.2-dev now has both the original x86 BIOS prototype and a first AArch64 mobile-foundation target for QEMU `virt`. It is not yet a production mobile OS and is not ready to flash to a phone.

## What works now

### x86 prototype
- BIOS-style 512-byte boot sector
- 32-bit protected-mode kernel
- VGA text console
- PS/2 keyboard input
- Interactive `vipos>` shell
- `.vip` application package format v1
- Tiny VIP Bytecode v1 runtime
- GitHub Actions headless QEMU boot test

### AArch64 mobile foundation
- 64-bit AArch64 kernel entry
- QEMU `virt` board target
- PL011 UART driver
- Serial shell
- `.vip` package header visibility
- DTB pointer preserved for upcoming device-tree/HAL work
- Separate GitHub Actions AArch64 build + QEMU boot test
- MMU intentionally disabled for this first bring-up milestone

## Quick start — x86

```bash
make clean all
make check
qemu-system-i386 -drive format=raw,file=vipos.img
```

## Quick start — AArch64

An LLVM/LLD toolchain with AArch64 support is used by CI:

```bash
make arm64
make arm64-check
qemu-system-aarch64 \
  -M virt -cpu cortex-a53 -m 256M \
  -nographic -monitor none \
  -serial stdio \
  -kernel build/vipos-arm64.elf
```

Expected boot markers:

```text
VIPOS_KERNEL_OK
VIPOS_ARM64_OK
```

## `.vip` package format

VIPOS uses `.vip` as its application package extension.

The v1 binary header contains magic `VIP0`, version, header size, manifest size, payload size, payload type and a SHA-256 payload digest.

Build/inspect the demo package:

```bash
make vip
python3 tools/inspectvip.py build/demo.vip
```

## Repository structure

```text
VIPOS/
├── .github/workflows/ci.yml
├── arch/arm64/
│   ├── kmain.c
│   ├── linker.ld
│   └── start.S
├── boot/boot.S
├── docs/VIPOS_ARCHITECTURE.md
├── include/vipos.h
├── kernel/
│   ├── entry.S
│   ├── kmain.c
│   └── linker.ld
├── samples/
├── tools/
├── Makefile
└── README.md
```

## Roadmap

### VIPOS 0.2-dev — current
- AArch64 kernel foundation
- QEMU ARM `virt`
- PL011 serial console
- device-tree/HAL groundwork

### VIPOS 0.3
- framebuffer console
- MMU + page tables
- exception/interrupt vectors
- timer + preemptive scheduler
- kernel heap
- process/address-space primitives

### VIPOS 0.4+
- touchscreen/input framework
- filesystem/storage
- IPC/system services
- GUI/compositor
- app permissions/signing
- audio, Wi-Fi, Bluetooth, USB, sensors, camera, power
- OTA/recovery
- eventually a port to one real phone model

## Important

The current AArch64 target is a QEMU development platform, not a phone firmware image. A real phone port needs a specific SoC/device, boot-chain analysis and device-specific drivers.
