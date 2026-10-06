.code16
.intel_syntax noprefix
.section .text.entry
.global _start
.extern kmain

_start:
    cli

    # The kernel was loaded at physical 0x8000 and entered as 0800:0000.
    # Use a zero-based data segment so the real-mode LGDT operand
    # resolves directly to the kernel physical/link address (0x8000+).
    xor ax, ax
    mov ss, ax
    mov sp, 0x7000
    xor ax, ax
    mov ds, ax
    mov es, ax

    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_entry

.code32
protected_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x70000

    call kmain
1:
    cli
    hlt
    jmp 1b

.align 8
gdt:
    # null
    .quad 0x0000000000000000
    # 0x08: flat 32-bit code
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x9A, 0xCF, 0x00
    # 0x10: flat 32-bit data
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x92, 0xCF, 0x00

gdt_descriptor:
    .word gdt_descriptor_end - gdt - 1
    .long gdt

gdt_descriptor_end:

.section .note.GNU-stack,"",@progbits
