.section .text.boot
.global _start
.extern arm64_kmain

.equ STACK_TOP, 0x40200000

_start:
    // QEMU virt enters with MMU off. Keep execution in physical == virtual space.
    // x0 holds the DTB address; preserve it for future hardware discovery work.
    mov x19, x0

    // Set a known stack inside the QEMU guest RAM region.
    ldr x1, =STACK_TOP
    mov sp, x1

    // Exception level is left at whatever firmware/QEMU provided.
    // For this prototype we run as-is and do not enable the MMU yet.
    bl arm64_kmain

1:
    wfe
    b 1b

.section .note.GNU-stack,"",@progbits
