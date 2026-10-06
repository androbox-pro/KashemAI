# VIPOS Architecture

```text
Applications (.vip)
        |
        v
VIPOS UI / Shell
        |
        v
System Services
        |
        v
VIPOS Framework / Runtime
        |
        v
Hardware Abstraction Layer
        |
        v
AArch64 / x86 Kernel + Drivers
        |
        v
Boot / Firmware Interface
        |
        v
Hardware / Virtual Hardware
```

## Current implementation

VIPOS has two development targets:

- x86 BIOS prototype: 32-bit protected-mode kernel, VGA/PS2 console and the original interactive shell.
- AArch64 mobile foundation: 64-bit kernel entry, QEMU `virt`, PL011 serial console, DTB pointer hand-off and embedded `.vip` package data.

The AArch64 target intentionally keeps the MMU, interrupt controller, timer scheduler and device-tree parser for the next milestones.

## Design principles

1. Keep application packaging independent from low-level hardware.
2. Put hardware-specific code behind a platform/HAL boundary.
3. Validate `.vip` payload integrity before real third-party installation/execution.
4. Build memory management, scheduling and IPC as reusable kernel primitives.
5. Treat each physical phone model as a separate device port with its own boot chain and drivers.
