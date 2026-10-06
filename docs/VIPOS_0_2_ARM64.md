# VIPOS 0.2 — AArch64 Bring-up

## Goal

Move the VIPOS kernel foundation from the x86 BIOS prototype toward the CPU architecture used by modern mobile hardware.

## 0.2 architecture

```text
.vip app / tools
      |
VIPOS shell / framework
      |
AArch64 kernel
  ├─ UART console
  ├─ HAL/platform layer (next)
  ├─ exception vectors (next)
  ├─ timer/scheduler (next)
  └─ MMU/page tables (next)
      |
QEMU virt / future real SoC
```

## Current boot contract

QEMU's `virt` machine supplies a device-tree address in AArch64 register `x0`. The prototype preserves that address so later code can parse the DTB and discover devices without hard-coding them.

The first-stage platform driver currently uses the QEMU `virt` PL011 UART at `0x09000000`.

## Safety boundary

This milestone deliberately does not attempt to boot on a real phone. No real-phone flashing, modem bring-up or proprietary boot-chain handling is included here.
