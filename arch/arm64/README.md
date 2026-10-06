# VIPOS 0.2 AArch64 target

This target is the first mobile-oriented architecture bring-up for VIPOS.
It boots as a standalone freestanding AArch64 ELF under QEMU's `virt` machine.

Current scope:

- AArch64 entry point
- known kernel stack
- PL011 UART console
- serial boot markers for CI
- simple serial command shell
- bundled `.vip` package embedded into the kernel ELF
- DTB pointer preserved for later device-tree/HAL work
- MMU disabled for this bring-up stage

Build locally with an AArch64-capable LLVM toolchain:

```bash
make arm64
make arm64-check
```

Run with QEMU:

```bash
qemu-system-aarch64 \
  -M virt -cpu cortex-a53 -m 256M \
  -nographic -monitor none \
  -serial stdio \
  -kernel build/vipos-arm64.elf
```

Expected startup markers:

```text
VIPOS_KERNEL_OK
VIPOS_ARM64_OK
VIPOS Mobile Foundation booted on AArch64.
```
